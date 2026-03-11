use crate::{
    CoCo, CoCoError,
    model::{Class, CoCoEvent, Object, Property, Rule, TimedValue, Value},
    server::secure::{Database, DatabaseError, User, create_jwt, verify_jwt},
};
use async_trait::async_trait;
use axum::{
    Extension, Json, Router,
    extract::{
        Path, Query, Request, State, WebSocketUpgrade,
        ws::{Message, WebSocket},
    },
    http::{StatusCode, header},
    middleware::{Next, from_fn_with_state},
    response::{IntoResponse, Response},
    routing::{get, patch, post},
};
use chrono::{DateTime, Utc};
use serde::Deserialize;
use std::{collections::HashMap, sync::Arc};
pub use tower_http;
use tracing::trace;
use utoipa::openapi::security::{HttpAuthScheme, HttpBuilder, SecurityScheme};
use utoipa::{IntoParams, Modify, OpenApi, ToSchema};

type OpenApiValue = Value;
type OpenApiObject = Object;

struct SecurityAddon;

impl Modify for SecurityAddon {
    fn modify(&self, openapi: &mut utoipa::openapi::OpenApi) {
        let components = openapi.components.get_or_insert_with(Default::default);
        components.add_security_scheme("bearerAuth", SecurityScheme::Http(HttpBuilder::new().scheme(HttpAuthScheme::Bearer).bearer_format("JWT").build()));
    }
}

#[async_trait]
pub trait CoCoState: Clone + Send + Sync + 'static {
    fn coco(&self) -> Arc<CoCo>;
    fn users_db(&self) -> Arc<dyn Database>;
}

pub fn build_coco_router<S>(state: S) -> Router<S>
where
    S: CoCoState,
{
    let protected_routes = Router::new()
        .route("/users", get(get_users::<S>).post(create_user::<S>))
        .route("/classes", post(create_class::<S>))
        .route("/objects", post(create_object::<S>))
        .route("/objects/{id}", patch(set_properties::<S>))
        .route("/objects/{id}/data", post(add_data::<S>))
        .route("/rules", post(create_rule::<S>))
        .route_layer(from_fn_with_state(state.clone(), auth_middleware::<S>));

    Router::new()
        .route("/register", post(register::<S>))
        .route("/login", post(login::<S>))
        .route("/ws", get(ws_handler::<S>))
        .route("/classes", get(get_classes::<S>))
        .route("/classes/{name}", get(get_class::<S>))
        .route("/objects", get(get_objects::<S>))
        .route("/objects/{id}", get(get_object::<S>))
        .route("/objects/{id}/data", get(get_data::<S>))
        .route("/rules", get(get_rules::<S>))
        .route("/rules/{name}", get(get_rule::<S>))
        .route("/openapi", get(openapi))
        .merge(protected_routes)
}

#[derive(Deserialize, ToSchema)]
struct Credentials {
    username: String,
    password: String,
}

#[derive(Debug, Clone)]
struct CurrentUser {
    _id: String,
    role: String,
}

async fn auth_middleware<S: CoCoState>(State(state): State<S>, mut req: Request, next: Next) -> Result<Response, StatusCode> {
    let header = req.headers().get(header::AUTHORIZATION).and_then(|h| h.to_str().ok());
    if let Some(token) = header.and_then(|h| h.strip_prefix("Bearer "))
        && let Ok(claims) = verify_jwt(token, state.users_db().secret().to_string())
    {
        req.extensions_mut().insert(CurrentUser { _id: claims.sub, role: claims.role });
        return Ok(next.run(req).await);
    }
    Err(StatusCode::UNAUTHORIZED)
}

#[utoipa::path(
        post,
        path = "/login",
        tag = "Authentication",
        summary = "Login a user",
        description = "Authenticate a user with their username and password, returns a JWT token if successful.",
        request_body = Credentials,
        responses(
            (status = 200, description = "User authenticated successfully, returns JWT token"),
            (status = 401, description = "Invalid username or password"),
            (status = 500, description = "Failed to authenticate user")
        )
    )]
async fn login<S: CoCoState>(State(state): State<S>, Json(req): Json<Credentials>) -> impl IntoResponse {
    let user = state.users_db().get_user(&req.username, &req.password).await;
    match user {
        Ok(user) => create_jwt(&user.username, &user.role, state.users_db().secret()).map_err(|_| StatusCode::INTERNAL_SERVER_ERROR),
        Err(DatabaseError::Unauthorized(_)) => Err(StatusCode::UNAUTHORIZED),
        Err(_) => Err(StatusCode::INTERNAL_SERVER_ERROR),
    }
}

#[utoipa::path(
        post,
        path = "/register",
        tag = "Authentication",
        summary = "Register a new user",
        description = "Create a new user account with a username, password, and role.",
        request_body = Credentials,
        responses(
            (status = 200, description = "User registered successfully, returns JWT token"),
            (status = 409, description = "Username already exists"),
            (status = 500, description = "Failed to register user")
        )
    )]
async fn register<S: CoCoState>(State(state): State<S>, Json(req): Json<Credentials>) -> impl IntoResponse {
    match state.users_db().create_user(&req.username, &req.password, "user").await {
        Ok(_) => create_jwt(&req.username, "user", state.users_db().secret()).map_err(|_| StatusCode::INTERNAL_SERVER_ERROR),
        Err(DatabaseError::UserAlreadyExists(_)) => Err(StatusCode::CONFLICT),
        Err(_) => Err(StatusCode::INTERNAL_SERVER_ERROR),
    }
}

#[utoipa::path(
        get,
        path = "/users",
        tag = "Authentication",
        summary = "List all users",
        description = "Retrieve a list of all registered users (admin only).",
        security(("bearerAuth" = [])),
        responses(
            (status = 200, description = "List of users", body = [User]),
            (status = 401, description = "Missing or invalid JWT token"),
            (status = 403, description = "Forbidden - only admin users can view the list of users"),
            (status = 500, description = "Failed to retrieve users")
        )
    )]
async fn get_users<S: CoCoState>(State(state): State<S>, Extension(user): Extension<CurrentUser>) -> impl IntoResponse {
    if user.role != "admin" {
        return (StatusCode::FORBIDDEN, "Only admin users can view the list of users").into_response();
    }
    match state.users_db().get_users().await {
        Ok(users) => Json(users).into_response(),
        Err(_) => (StatusCode::INTERNAL_SERVER_ERROR, "Failed to retrieve users").into_response(),
    }
}

#[utoipa::path(
        post,
        path = "/users",
        tag = "Authentication",
        summary = "Create a new user",
        description = "Create a new user account with a username, password, and role (admin only).",
        request_body = Credentials,
        security(("bearerAuth" = [])),
        responses(
            (status = 201, description = "User created successfully"),
            (status = 401, description = "Missing or invalid JWT token"),
            (status = 403, description = "Forbidden - only admin users can create new users"),
            (status = 409, description = "Username already exists"),
            (status = 500, description = "Failed to create user")
        )
    )]
async fn create_user<S: CoCoState>(State(state): State<S>, Extension(user): Extension<CurrentUser>, Json(req): Json<Credentials>) -> impl IntoResponse {
    if user.role != "admin" {
        return (StatusCode::FORBIDDEN, "Only admin users can create new users").into_response();
    }
    match state.users_db().create_user(&req.username, &req.password, "user").await {
        Ok(_) => (StatusCode::CREATED, "User created successfully").into_response(),
        Err(DatabaseError::UserAlreadyExists(_)) => (StatusCode::CONFLICT, "Username already exists").into_response(),
        Err(_) => (StatusCode::INTERNAL_SERVER_ERROR, "Failed to create user").into_response(),
    }
}

#[utoipa::path(
        get,
        path = "/classes",
        tag = "Classes",
        summary = "List all classes",
        description = "Retrieve a list of all available classes in the knowledge base.",
        responses(
            (status = 200, description = "List of classes", body = [Class])
        )
    )]
async fn get_classes<S: CoCoState>(State(state): State<S>) -> impl IntoResponse {
    trace!("Handling request to list all classes");
    Json(state.coco().get_classes().await)
}

#[utoipa::path(
        get,
        path = "/classes/{name}",
        tag = "Classes",
        summary = "Get a class",
        description = "Retrieve details for a specific class by its name.",
        params(
            ("name" = String, Path, description = "Name of the class to retrieve")
        ),
        responses(
            (status = 200, description = "The requested class", body = Class),
            (status = 404, description = "Class not found")
        )
    )]
async fn get_class<S: CoCoState>(Path(name): Path<String>, State(state): State<S>) -> impl IntoResponse {
    trace!("Handling request to get class '{}'", name);
    match state.coco().get_class(&name).await {
        // We clone the single class to safely return it
        Some(class) => Json(class).into_response(),
        None => (StatusCode::NOT_FOUND, "Class not found").into_response(),
    }
}

#[utoipa::path(
        post,
        path = "/classes",
        tag = "Classes",
        summary = "Create a class",
        description = "Create a new class in the knowledge base.",
        request_body = Class,
        security(("bearerAuth" = [])),
        responses(
            (status = 201, description = "Class created successfully"),
            (status = 401, description = "Missing or invalid JWT token"),
            (status = 403, description = "Forbidden - only admin users can create classes"),
            (status = 409, description = "Class already exists"),
            (status = 500, description = "Failed to create class")
        )
    )]
async fn create_class<S: CoCoState>(State(state): State<S>, Extension(user): Extension<CurrentUser>, Json(class): Json<Class>) -> impl IntoResponse {
    if user.role != "admin" {
        return (StatusCode::FORBIDDEN, "Only admin users can create classes").into_response();
    }
    trace!("Handling request to create class '{}'", class.name);
    match state.coco().create_class(class).await {
        Ok(_) => StatusCode::CREATED.into_response(),
        Err(e) => match e {
            CoCoError::ClassAlreadyExists(msg) => (StatusCode::CONFLICT, format!("Class {} already exists", msg)).into_response(),
            _ => (StatusCode::INTERNAL_SERVER_ERROR, "Failed to create class").into_response(),
        },
    }
}

#[derive(Deserialize, IntoParams)]
#[into_params(parameter_in = Query)]
struct ObjectFilter {
    class: Option<String>,
    #[serde(flatten)]
    extra: Option<HashMap<String, String>>,
}

#[utoipa::path(
        get,
        path = "/objects",
        tag = "Objects",
        summary = "List all objects",
        description = "Retrieve a list of all available objects in the knowledge base.",
        params(ObjectFilter),
        responses(
            (status = 200, description = "List of objects", body = [OpenApiObject])
        )
    )]
async fn get_objects<S: CoCoState>(State(state): State<S>, Query(params): Query<ObjectFilter>) -> impl IntoResponse {
    trace!("Handling request to list objects with filter: class={:?}, extra={:?}", params.class, params.extra);
    let objects = state.coco().get_objects().await;
    let filtered_objects: Vec<OpenApiObject> = objects
        .into_iter()
        .filter(|o| {
            let class_match = params.class.as_ref().is_none_or(|class_name| o.classes.contains(class_name));
            let extra_match = params.extra.as_ref().is_none_or(|extra| extra.iter().all(|(k, v)| o.properties.as_ref().and_then(|props| props.get(k)).is_none_or(|prop| prop == v)));
            class_match && extra_match
        })
        .collect();
    Json(filtered_objects).into_response()
}

#[utoipa::path(
        get,
        path = "/objects/{id}",
        tag = "Objects",
        summary = "Get an object",
        description = "Retrieve details for a specific object by its ID.",
        params(
            ("id" = String, Path, description = "ID of the object to retrieve")
        ),
        responses(
            (status = 200, description = "The requested object", body = OpenApiObject),
            (status = 404, description = "Object not found")
        )
    )]
async fn get_object<S: CoCoState>(Path(id): Path<String>, State(state): State<S>) -> impl IntoResponse {
    trace!("Handling request to get object with ID '{}'", id);
    match state.coco().get_object(&id).await {
        Some(object) => Json(object).into_response(),
        None => (StatusCode::NOT_FOUND, "Object not found").into_response(),
    }
}

#[utoipa::path(
        post,
        path = "/objects",
        tag = "Objects",
        summary = "Create an object",
        description = "Create a new object in the knowledge base.",
        request_body = OpenApiObject,
        security(("bearerAuth" = [])),
        responses(
            (status = 201, description = "Object created successfully", body = String),
            (status = 401, description = "Missing or invalid JWT token"),
            (status = 403, description = "Forbidden - only admin users can create objects"),
            (status = 500, description = "Failed to create object")
        )
    )]
async fn create_object<S: CoCoState>(State(state): State<S>, Extension(user): Extension<CurrentUser>, Json(object): Json<OpenApiObject>) -> impl IntoResponse {
    if user.role != "admin" {
        return (StatusCode::FORBIDDEN, "Only admin users can create objects").into_response();
    }
    trace!("Handling request to create object with ID '{:?}'", object.id);
    match state.coco().create_object(object).await {
        Ok(object_id) => (StatusCode::CREATED, object_id).into_response(),
        Err(e) => match e {
            CoCoError::ObjectAlreadyExists(msg) => (StatusCode::CONFLICT, format!("Object {} already exists", msg)).into_response(),
            _ => (StatusCode::INTERNAL_SERVER_ERROR, "Failed to create object").into_response(),
        },
    }
}

#[utoipa::path(
        patch,
        path = "/objects/{id}",
        tag = "Objects",
        summary = "Set object properties",
        description = "Update the properties of an existing object.",
        params(
            ("id" = String, Path, description = "ID of the object to update")
        ),
        request_body = inline(HashMap<String, Value>),
        security(("bearerAuth" = [])),
        responses(
            (status = 200, description = "Object properties updated successfully"),
            (status = 401, description = "Missing or invalid JWT token"),
            (status = 403, description = "Forbidden - only admin users can update object properties"),
            (status = 404, description = "Object not found"),
            (status = 500, description = "Failed to update object properties")
        )
    )]
async fn set_properties<S: CoCoState>(State(state): State<S>, Extension(user): Extension<CurrentUser>, Path(id): Path<String>, Json(properties): Json<HashMap<String, Value>>) -> impl IntoResponse {
    if user.role != "admin" {
        return (StatusCode::FORBIDDEN, "Only admin users can update object properties").into_response();
    }
    trace!("Handling request to set properties for object with ID '{}'", id);
    match state.coco().set_properties(&id, properties).await {
        Ok(_) => StatusCode::OK.into_response(),
        Err(e) => match e {
            CoCoError::ObjectNotFound(msg) => (StatusCode::NOT_FOUND, format!("Object {} not found", msg)).into_response(),
            _ => (StatusCode::INTERNAL_SERVER_ERROR, "Failed to update object properties").into_response(),
        },
    }
}

#[derive(Deserialize)]
struct DateQuery {
    time: Option<DateTime<Utc>>,
}

#[utoipa::path(
        post,
        path = "/objects/{id}/data",
        tag = "Objects",
        summary = "Add data to an object",
        description = "Add new data values to an existing object.",
        params(
            ("id" = String, Path, description = "ID of the object to update"),
            ("time" = Option<DateTime<Utc>>, Query, description = "Timestamp for the data being added (optional, defaults to current time)")
        ),
        request_body = inline(HashMap<String, Value>),
        security(("bearerAuth" = [])),
        responses(
            (status = 200, description = "Data added to object successfully"),
            (status = 401, description = "Missing or invalid JWT token"),
            (status = 403, description = "Forbidden - only admin users can add data to objects"),
            (status = 404, description = "Object not found"),
            (status = 500, description = "Failed to add data to object")
        )
    )]
async fn add_data<S: CoCoState>(State(state): State<S>, Extension(user): Extension<CurrentUser>, Path(object_id): Path<String>, Query(date_time): Query<DateQuery>, Json(values): Json<HashMap<String, Value>>) -> impl IntoResponse {
    if user.role != "admin" {
        return (StatusCode::FORBIDDEN, "Only admin users can add data to objects").into_response();
    }
    trace!("Handling request to add data to object with ID '{}'", object_id);
    match state.coco().add_data(&object_id, values, date_time.time.unwrap_or_else(Utc::now)).await {
        Ok(_) => StatusCode::OK.into_response(),
        Err(e) => match e {
            CoCoError::ObjectNotFound(msg) => (StatusCode::NOT_FOUND, format!("Object {} not found", msg)).into_response(),
            _ => (StatusCode::INTERNAL_SERVER_ERROR, "Failed to add data to object").into_response(),
        },
    }
}

#[derive(Deserialize)]
struct DataFilter {
    start: Option<DateTime<Utc>>,
    end: Option<DateTime<Utc>>,
}

#[utoipa::path(
        get,
        path = "/objects/{id}/data",
        tag = "Objects",
        summary = "Get object data",
        description = "Retrieve data values for a specific object, optionally filtered by a time range.",
        params(
            ("id" = String, Path, description = "ID of the object to retrieve data for"),
            ("start" = Option<DateTime<Utc>>, Query, description = "Start of the time range filter (optional)"),
            ("end" = Option<DateTime<Utc>>, Query, description = "End of the time range filter (optional)")
        ),
        responses(
            (status = 200, description = "List of data values for the object", body = [HashMap<String, Value>]),
            (status = 404, description = "Object not found"),
            (status = 500, description = "Failed to retrieve object data")
        )
    )]
async fn get_data<S: CoCoState>(State(state): State<S>, Path(object_id): Path<String>, Query(filter): Query<DataFilter>) -> impl IntoResponse {
    trace!("Handling request to get data for object with ID '{}' with filter: start={:?}, end={:?}", object_id, filter.start, filter.end);
    match state.coco().get_data(&object_id, filter.start, filter.end).await {
        Ok(data) => {
            let mut result: HashMap<String, Vec<TimedValue>> = HashMap::new();
            for (map, timestamp) in data {
                for (key, value) in map {
                    result.entry(key).or_default().push(TimedValue { value, timestamp });
                }
            }
            Json(result).into_response()
        }
        Err(e) => match e {
            CoCoError::ObjectNotFound(msg) => (StatusCode::NOT_FOUND, format!("Object {} not found", msg)).into_response(),
            _ => (StatusCode::INTERNAL_SERVER_ERROR, "Failed to retrieve object data").into_response(),
        },
    }
}

#[utoipa::path(
        get,
        path = "/rules",
        tag = "Rules",
        summary = "List all rules",
        description = "Retrieve a list of all available rules in the knowledge base.",
        responses(
            (status = 200, description = "List of rules", body = [String])
        )
    )]
async fn get_rules<S: CoCoState>(State(state): State<S>) -> impl IntoResponse {
    trace!("Handling request to list all rules");
    Json(state.coco().get_rules().await).into_response()
}

#[utoipa::path(
        get,
        path = "/rules/{name}",
        tag = "Rules",
        summary = "Get a rule",
        description = "Retrieve details for a specific rule by its name.",
        params(
            ("name" = String, Path, description = "Name of the rule to retrieve")
        ),
        responses(
            (status = 200, description = "The requested rule", body = String),
            (status = 404, description = "Rule not found")
        )
    )]
async fn get_rule<S: CoCoState>(Path(name): Path<String>, State(state): State<S>) -> impl IntoResponse {
    trace!("Handling request to get rule '{}'", name);
    match state.coco().get_rule(&name).await {
        Some(rule) => Json(rule).into_response(),
        None => (StatusCode::NOT_FOUND, "Rule not found").into_response(),
    }
}

#[utoipa::path(
        post,
        path = "/rules",
        tag = "Rules",
        summary = "Create a rule",
        description = "Create a new rule in the knowledge base.",
        request_body = Rule,
        security(("bearerAuth" = [])),
        responses(
            (status = 201, description = "Rule created successfully"),
            (status = 401, description = "Missing or invalid JWT token"),
            (status = 403, description = "Forbidden - only admin users can create rules"),
            (status = 500, description = "Failed to create rule")
        )
    )]
async fn create_rule<S: CoCoState>(State(state): State<S>, Extension(user): Extension<CurrentUser>, Json(rule): Json<Rule>) -> impl IntoResponse {
    if user.role != "admin" {
        return (StatusCode::FORBIDDEN, "Only admin users can create rules").into_response();
    }
    trace!("Handling request to create rule '{}'", rule.name);
    match state.coco().create_rule(rule).await {
        Ok(_) => StatusCode::CREATED.into_response(),
        Err(e) => match e {
            CoCoError::RuleAlreadyExists(msg) => (StatusCode::CONFLICT, format!("Rule {} already exists", msg)).into_response(),
            _ => (StatusCode::INTERNAL_SERVER_ERROR, "Failed to create rule").into_response(),
        },
    }
}

#[utoipa::path(
        get,
        path = "/ws",
        tag = "System",
        summary = "WebSocket connection",
        description = "Establish a WebSocket connection for real-time updates.",
        responses(
            (status = 101, description = "WebSocket connection established"),
        )
    )]
async fn ws_handler<S: CoCoState>(ws: WebSocketUpgrade, State(state): State<S>) -> impl IntoResponse {
    ws.on_upgrade(move |socket| handle_socket(socket, state.coco().clone()))
}

async fn handle_socket(mut socket: WebSocket, coco: Arc<CoCo>) {
    let classes_map: std::collections::HashMap<String, serde_json::Value> = coco
        .get_classes()
        .await
        .into_iter()
        .map(|mut c| {
            let name = std::mem::take(&mut c.name);
            let mut v = serde_json::to_value(&c).unwrap();
            v.as_object_mut().unwrap().remove("name");
            (name, v)
        })
        .collect();
    let objects_map: std::collections::HashMap<String, serde_json::Value> = coco
        .get_objects()
        .await
        .into_iter()
        .map(|mut o| {
            let id = o.id.take().unwrap();
            let mut v = serde_json::to_value(&o).unwrap();
            v.as_object_mut().unwrap().remove("id");
            (id, v)
        })
        .collect();
    let rules_map: std::collections::HashMap<String, serde_json::Value> = coco
        .get_rules()
        .await
        .into_iter()
        .map(|mut r| {
            let name = std::mem::take(&mut r.name);
            let mut v = serde_json::to_value(&r).unwrap();
            v.as_object_mut().unwrap().remove("name");
            (name, v)
        })
        .collect();
    let init_msg = serde_json::json!({
        "msg_type": "coco",
        "classes": classes_map,
        "objects": objects_map,
        "rules": rules_map
    });
    socket.send(Message::Text(serde_json::to_string(&init_msg).unwrap().into())).await.ok();

    let mut rx = coco.get_event_sender().subscribe();
    while let Some(msg) = rx.recv().await {
        let send_result = match msg {
            CoCoEvent::ClassCreated(class_name) => {
                trace!("Received event: ClassCreated for class '{}'", class_name);
                let mut update_msg = serde_json::to_value(coco.get_class(&class_name).await).unwrap();
                update_msg["msg_type"] = serde_json::json!("class_created");
                socket.send(Message::Text(serde_json::to_string(&update_msg).unwrap().into())).await
            }
            CoCoEvent::ObjectCreated(object_id) => {
                trace!("Received event: ObjectCreated for object '{}'", object_id);
                let mut update_msg = serde_json::to_value(coco.get_object(&object_id).await).unwrap();
                update_msg["msg_type"] = serde_json::json!("object_created");
                socket.send(Message::Text(serde_json::to_string(&update_msg).unwrap().into())).await
            }
            CoCoEvent::AddedClass(object_id, class_name) => {
                trace!("Received event: AddedClass - object '{}', class '{}'", object_id, class_name);
                let update_msg = serde_json::json!({
                    "msg_type": "added_class",
                    "object_id": object_id,
                    "class_name": class_name
                });
                socket.send(Message::Text(serde_json::to_string(&update_msg).unwrap().into())).await
            }
            CoCoEvent::UpdatedProperties(object_id, properties) => {
                trace!("Received event: UpdatedProperties for object '{}'", object_id);
                let update_msg = serde_json::json!({
                    "msg_type": "updated_properties",
                    "object_id": object_id,
                    "properties": properties
                });
                socket.send(Message::Text(serde_json::to_string(&update_msg).unwrap().into())).await
            }
            CoCoEvent::AddedValues(object_id, values, date_time) => {
                trace!("Received event: AddedValues for object '{}'", object_id);
                let update_msg = serde_json::json!({
                    "msg_type": "added_values",
                    "object_id": object_id,
                    "values": values,
                    "date_time": date_time
                });
                socket.send(Message::Text(serde_json::to_string(&update_msg).unwrap().into())).await
            }
            CoCoEvent::RuleCreated(rule) => {
                trace!("Received event: RuleCreated for rule '{}'", rule);
                let mut update_msg = serde_json::to_value(coco.get_rule(&rule).await).unwrap();
                update_msg["msg_type"] = serde_json::json!("rule_created");
                socket.send(Message::Text(serde_json::to_string(&update_msg).unwrap().into())).await
            }
            _ => Ok(()),
        };

        // If sending fails (e.g., client disconnected), break out of the loop
        if send_result.is_err() {
            break;
        }
    }
}

#[utoipa::path(
        get,
        path = "/openapi",
        tag = "System",
        summary = "Get OpenAPI spec",
        description = "Retrieve the OpenAPI specification for this API.",
        responses(
            (status = 200, description = "OpenAPI specification in JSON format", body = String)
        )
    )]
async fn openapi() -> impl IntoResponse {
    Json(ApiDoc::openapi())
}

#[derive(OpenApi)]
#[openapi(
    paths(get_users, create_user, register, login, get_classes, get_class, create_class, get_objects, get_object, create_object, set_properties, add_data, get_data, get_rules, get_rule, create_rule, ws_handler, openapi),
    components(
        schemas(Class, Rule, Property, OpenApiObject, OpenApiValue, User)
    ),
    modifiers(&SecurityAddon),
    tags(
        (name = "Authentication", description = "Endpoints for user registration and login"),
        (name = "Classes", description = "Operations related to knowledge base classes"),
        (name = "Objects", description = "Operations related to knowledge base objects"),
        (name = "Rules", description = "Operations related to knowledge base rules"),
        (name = "System", description = "System and utility endpoints")
    )
)]
pub struct ApiDoc;

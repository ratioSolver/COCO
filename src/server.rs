use axum::{
    Router,
    extract::{
        Path, State, WebSocketUpgrade,
        ws::{Message, WebSocket},
    },
    http::StatusCode,
    response::IntoResponse,
    routing::get,
};
use std::{collections::HashMap, sync::Arc};
use utoipa::OpenApi;

use crate::{
    CoCo, CoCoError, CoCoState,
    model::{Class, CoCoEvent, Object, Rule, Value},
};

pub fn build_coco_router<S>() -> Router<S>
where
    S: CoCoState,
{
    Router::new()
        .route("/ws", get(ws_handler::<S>))
        .route("/classes", get(get_classes::<S>).post(create_class::<S>))
        .route("/classes/{name}", get(get_class::<S>))
        .route("/objects", get(get_objects::<S>).post(create_object::<S>))
        .route("/objects/{id}", get(get_object::<S>).patch(set_properties::<S>))
        .route("/rules", get(get_rules::<S>).post(create_rule::<S>))
        .route("/rules/{name}", get(get_rule::<S>))
        .route("/openapi", get(openapi))
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
    axum::Json(state.coco().get_classes().await)
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
    match state.coco().get_class(&name).await {
        // We clone the single class to safely return it
        Some(class) => axum::Json(class).into_response(),
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
        responses(
            (status = 201, description = "Class created successfully"),
            (status = 500, description = "Failed to create class")
        )
    )]
async fn create_class<S: CoCoState>(State(state): State<S>, axum::Json(class): axum::Json<Class>) -> impl IntoResponse {
    match state.coco().create_class(class).await {
        Ok(_) => StatusCode::CREATED.into_response(),
        Err(e) => match e {
            CoCoError::ClassAlreadyExists(msg) => (StatusCode::CONFLICT, msg).into_response(),
            _ => (StatusCode::INTERNAL_SERVER_ERROR, format!("Failed to create class")).into_response(),
        },
    }
}

#[utoipa::path(
        get,
        path = "/objects",
        tag = "Objects",
        summary = "List all objects",
        description = "Retrieve a list of all available objects in the knowledge base.",
        responses(
            (status = 200, description = "List of objects", body = [Object])
        )
    )]
async fn get_objects<S: CoCoState>(State(state): State<S>) -> impl IntoResponse {
    axum::Json(state.coco().get_objects().await)
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
            (status = 200, description = "The requested object", body = Object),
            (status = 404, description = "Object not found")
        )
    )]
async fn get_object<S: CoCoState>(Path(id): Path<String>, State(state): State<S>) -> impl IntoResponse {
    match state.coco().get_object(&id).await {
        Some(object) => axum::Json(object).into_response(),
        None => (StatusCode::NOT_FOUND, "Object not found").into_response(),
    }
}

#[utoipa::path(
        post,
        path = "/objects",
        tag = "Objects",
        summary = "Create an object",
        description = "Create a new object in the knowledge base.",
        request_body = Object,
        responses(
            (status = 201, description = "Object created successfully", body = String),
            (status = 500, description = "Failed to create object")
        )
    )]
async fn create_object<S: CoCoState>(State(state): State<S>, axum::Json(object): axum::Json<Object>) -> impl IntoResponse {
    match state.coco().create_object(object).await {
        Ok(object_id) => (StatusCode::CREATED, object_id).into_response(),
        Err(e) => match e {
            CoCoError::ObjectAlreadyExists(msg) => (StatusCode::CONFLICT, msg).into_response(),
            _ => (StatusCode::INTERNAL_SERVER_ERROR, format!("Failed to create object")).into_response(),
        },
    }
}

#[utoipa::path(
        post,
        path = "/objects/{id}",
        tag = "Objects",
        summary = "Set object properties",
        description = "Update the properties of an existing object.",
        params(
            ("id" = String, Path, description = "ID of the object to update")
        ),
        request_body = HashMap<String, Value>,
        responses(
            (status = 200, description = "Object properties updated successfully"),
            (status = 500, description = "Failed to update object properties")
        )
    )]
async fn set_properties<S: CoCoState>(State(state): State<S>, Path(id): Path<String>, axum::Json(properties): axum::Json<HashMap<String, Value>>) -> impl IntoResponse {
    match state.coco().set_properties(&id, properties).await {
        Ok(_) => StatusCode::OK.into_response(),
        Err(e) => match e {
            CoCoError::ObjectNotFound(msg) => (StatusCode::NOT_FOUND, msg).into_response(),
            _ => (StatusCode::INTERNAL_SERVER_ERROR, format!("Failed to update object properties")).into_response(),
        },
    }
}

#[utoipa::path(
        get,
        path = "/rules",
        tag = "System",
        summary = "List all rules",
        description = "Retrieve a list of all available rules in the knowledge base.",
        responses(
            (status = 200, description = "List of rules", body = [String])
        )
    )]
async fn get_rules<S: CoCoState>(State(state): State<S>) -> impl IntoResponse {
    axum::Json(state.coco().get_rules().await).into_response()
}

#[utoipa::path(
        get,
        path = "/rules/{name}",
        tag = "System",
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
    match state.coco().get_rule(&name).await {
        Some(rule) => axum::Json(rule).into_response(),
        None => (StatusCode::NOT_FOUND, "Rule not found").into_response(),
    }
}

#[utoipa::path(
        post,
        path = "/rules",
        tag = "System",
        summary = "Create a rule",
        description = "Create a new rule in the knowledge base.",
        request_body = Rule,
        responses(
            (status = 201, description = "Rule created successfully"),
            (status = 500, description = "Failed to create rule")
        )
    )]
async fn create_rule<S: CoCoState>(State(state): State<S>, axum::Json(rule): axum::Json<Rule>) -> impl IntoResponse {
    match state.coco().create_rule(rule).await {
        Ok(_) => StatusCode::CREATED.into_response(),
        Err(e) => match e {
            CoCoError::RuleAlreadyExists(msg) => (StatusCode::CONFLICT, msg).into_response(),
            _ => (StatusCode::INTERNAL_SERVER_ERROR, format!("Failed to create rule")).into_response(),
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
        .map(|r| {
            let name = r.name;
            let mut v = serde_json::to_value(&r.content).unwrap();
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
    socket.send(Message::Text(serde_json::to_string(&init_msg).unwrap().into())).await.unwrap();

    let mut rx = coco.get_event_sender().subscribe();
    while let Ok(msg) = rx.recv().await {
        match msg {
            CoCoEvent::ClassCreated(class_name) => {
                let mut update_msg = serde_json::to_value(coco.get_class(&class_name).await).unwrap();
                update_msg["msg_type"] = serde_json::json!("class_created");
                socket.send(Message::Text(serde_json::to_string(&update_msg).unwrap().into())).await.unwrap();
            }
            CoCoEvent::ObjectCreated(object_id) => {
                let mut update_msg = serde_json::to_value(coco.get_object(&object_id).await).unwrap();
                update_msg["msg_type"] = serde_json::json!("object_created");
                socket.send(Message::Text(serde_json::to_string(&update_msg).unwrap().into())).await.unwrap();
            }
            CoCoEvent::AddedClass(object_id, class_name) => {
                let update_msg = serde_json::json!({
                    "msg_type": "added_class",
                    "object_id": object_id,
                    "class_name": class_name
                });
                socket.send(Message::Text(serde_json::to_string(&update_msg).unwrap().into())).await.unwrap();
            }
            CoCoEvent::UpdatedProperties(object_id, properties) => {
                let update_msg = serde_json::json!({
                    "msg_type": "updated_properties",
                    "object_id": object_id,
                    "properties": properties
                });
                socket.send(Message::Text(serde_json::to_string(&update_msg).unwrap().into())).await.unwrap();
            }
            CoCoEvent::AddedValues(object_id, values, date_time) => {
                let update_msg = serde_json::json!({
                    "msg_type": "added_values",
                    "object_id": object_id,
                    "values": values,
                    "date_time": date_time
                });
                socket.send(Message::Text(serde_json::to_string(&update_msg).unwrap().into())).await.unwrap();
            }
            CoCoEvent::RuleCreated(rule) => {
                let mut update_msg = serde_json::to_value(coco.get_rule(&rule).await).unwrap();
                update_msg["msg_type"] = serde_json::json!("rule_created");
                socket.send(Message::Text(serde_json::to_string(&update_msg).unwrap().into())).await.unwrap();
            }
            _ => {}
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
    axum::Json(ApiDoc::openapi())
}

#[derive(OpenApi)]
#[openapi(
    paths(get_classes, get_class, create_class, get_objects, get_object, create_object, ws_handler, openapi),
    tags(
        (name = "Classes", description = "Operations related to knowledge base classes"),
        (name = "Objects", description = "Operations related to knowledge base objects"),
        (name = "System", description = "System and utility endpoints")
    )
)]
pub struct ApiDoc;

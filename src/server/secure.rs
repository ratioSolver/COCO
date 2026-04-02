use std::collections::HashMap;

use crate::{
    CoCo,
    db::DatabaseError,
    model::{Class, Object, Property, Rule, Value},
};
use argon2::{
    Argon2, PasswordHash, PasswordHasher, PasswordVerifier,
    password_hash::{SaltString, rand_core::OsRng},
};
use axum::{
    Extension, Json, Router,
    extract::{Path, Query, Request, State},
    http::{StatusCode, header},
    middleware::{Next, from_fn_with_state},
    response::{IntoResponse, Response},
    routing::{get, patch, post},
};
use chrono::{Duration, Utc};
use futures::TryStreamExt;
use jsonwebtoken::{DecodingKey, EncodingKey, Header, Validation, errors::Error};
use mongodb::bson::doc;
use mongodb::{Client, IndexModel, bson::Document, options::IndexOptions};
use serde::{Deserialize, Serialize};
use tracing::{error, trace};
use utoipa::{
    IntoParams, Modify, OpenApi, ToSchema, openapi::security::{HttpAuthScheme, HttpBuilder, SecurityScheme}
};

type OpenApiValue = Value;
type OpenApiObject = Object;

#[derive(Clone, Serialize, Deserialize, Debug, ToSchema)]
pub struct User {
    username: String,
    password: String,
    pub role: String,
}

#[derive(Clone)]
pub struct UsersDB {
    name: String,
    secret: String,
    pub client: Client,
}

impl UsersDB {
    pub async fn new(name: String, secret: String, connection_string: String) -> Result<Self, DatabaseError> {
        let client = Client::with_uri_str(connection_string).await.map_err(|e| DatabaseError::ConnectionError(e.to_string()))?;
        let db = client.database(&name);
        let collection_names = db.list_collection_names().await.map_err(|e| DatabaseError::ConnectionError(e.to_string()))?;
        if collection_names.is_empty() {
            let users_collection = db.collection::<Document>("users");
            let index = IndexModel::builder().keys(doc! { "username": 1 }).options(IndexOptions::builder().unique(true).build()).build();
            users_collection.create_index(index).await.map_err(|e| DatabaseError::ConnectionError(e.to_string()))?;
            let initial_username = std::env::var("INITIAL_ADMIN_USERNAME").unwrap_or_else(|_| "admin".to_owned());
            let initial_password = std::env::var("INITIAL_ADMIN_PASSWORD").unwrap_or_else(|_| "admin".to_owned());
            users_collection.insert_one(doc! { "username": initial_username, "password": hash_password(initial_password.as_str()), "role": "admin" }).await.map_err(|e| DatabaseError::ConnectionError(e.to_string()))?;
        }
        Ok(UsersDB { name, secret, client })
    }

    pub fn name(&self) -> &str {
        &self.name
    }

    async fn get_users(&self) -> Result<Vec<User>, DatabaseError> {
        let db = self.client.database(&self.name);
        let collection = db.collection::<User>("users");
        let cursor = collection.find(doc! {}).await.map_err(|e| DatabaseError::ConnectionError(e.to_string()))?;
        let users: Vec<User> = cursor.try_collect().await.map_err(|e| DatabaseError::ConnectionError(e.to_string()))?;
        Ok(users)
    }

    async fn get_user(&self, username: &str, password: &str) -> Result<User, DatabaseError> {
        let db = self.client.database(&self.name);
        let users_collection = db.collection::<User>("users");
        let filter = doc! { "username": username };
        let user = users_collection.find_one(filter).await.map_err(|e| DatabaseError::NotFound(e.to_string()))?;
        if let Some(user) = user { if verify_password(password, &user.password) { Ok(user) } else { Err(DatabaseError::NotFound("Invalid username or password".to_string())) } } else { Err(DatabaseError::NotFound("Invalid username or password".to_string())) }
    }

    async fn create_user(&self, username: &str, password: &str, role: &str) -> Result<(), DatabaseError> {
        let db = self.client.database(&self.name);
        let collection = db.collection::<User>("users");
        let new_user = User { username: username.to_owned(), password: hash_password(password), role: role.to_owned() };
        collection.insert_one(new_user).await.map_err(|e| DatabaseError::ConnectionError(e.to_string()))?;
        Ok(())
    }
}

pub async fn setup_db() -> Result<UsersDB, DatabaseError> {
    let users_name = std::env::var("USERS_DB_NAME").unwrap_or_else(|_| "coco_users".to_string());
    let users_host = std::env::var("USERS_DB_HOST").unwrap_or_else(|_| "localhost".to_string());
    let users_port = std::env::var("USERS_DB_PORT").unwrap_or_else(|_| "27017".to_string()).parse().unwrap_or(27017);
    let users_uri = format!("mongodb://{}:{}/{}", users_host, users_port, users_name);
    let client = Client::with_uri_str(&users_uri).await.map_err(|e| DatabaseError::ConnectionError(format!("Failed to connect to users database: {}", e)))?;
    let secret = std::env::var("JWT_SECRET").unwrap_or_else(|_| "default_secret".to_owned());

    Ok(UsersDB { name: users_name, secret, client })
}

pub async fn secure_coco_router(coco: CoCo) -> Router {
    let db = setup_db().await.unwrap_or_else(|e| {
        error!("Failed to set up users database: {}", e);
        std::process::exit(1);
    });

    let protected_auth_router = Router::new().route("/users", get(get_users).post(create_user)).route_layer(from_fn_with_state(db.clone(), auth_middleware));
    let auth_router = Router::new().route("/register", post(register)).route("/login", post(login)).route("/refresh_token", post(refresh_token)).merge(protected_auth_router).with_state(db.clone());

    let protected_router = Router::new().route("/classes", post(create_class)).route_layer(from_fn_with_state(db, auth_middleware));
    let coco_router = Router::new().route("/classes", get(get_classes)).route("/classes/{name}", get(get_class)).route("/openapi", get(openapi)).merge(protected_router).with_state(coco);

    auth_router.merge(coco_router)
}

async fn auth_middleware(State(db): State<UsersDB>, mut req: Request, next: Next) -> Result<Response, StatusCode> {
    let header = req.headers().get(header::AUTHORIZATION).and_then(|h| h.to_str().ok());
    if let Some(token) = header.and_then(|h| h.strip_prefix("Bearer "))
        && let Ok(claims) = verify_jwt(token, &db.secret)
        && claims.token_type == "access"
    {
        req.extensions_mut().insert(CurrentUser { _id: claims.sub, role: claims.role });
        return Ok(next.run(req).await);
    }
    Err(StatusCode::UNAUTHORIZED)
}

pub fn hash_password(password: &str) -> String {
    let salt = SaltString::generate(&mut OsRng);
    Argon2::default().hash_password(password.as_bytes(), &salt).unwrap().to_string()
}

pub fn verify_password(password: &str, hash: &str) -> bool {
    let parsed_hash = PasswordHash::new(hash).unwrap();
    Argon2::default().verify_password(password.as_bytes(), &parsed_hash).is_ok()
}

#[derive(Debug, Serialize, Deserialize)]
pub struct Claims {
    sub: String,
    exp: usize,
    role: String,
    #[serde(default = "default_token_type")]
    token_type: String,
}

fn default_token_type() -> String {
    "access".to_owned()
}

pub fn create_jwt(user_id: &str, role: &str, secret: &str) -> Result<String, Error> {
    let now = Utc::now();
    let expire = now + Duration::hours(24);

    let claims = Claims {
        sub: user_id.to_owned(),
        exp: expire.timestamp() as usize,
        role: role.to_owned(),
        token_type: "access".to_owned(),
    };

    jsonwebtoken::encode(&Header::default(), &claims, &EncodingKey::from_secret(secret.as_ref()))
}

pub fn create_refresh_jwt(user_id: &str, role: &str, secret: &str) -> Result<String, Error> {
    let now = Utc::now();
    let expire = now + Duration::days(30);

    let claims = Claims {
        sub: user_id.to_owned(),
        exp: expire.timestamp() as usize,
        role: role.to_owned(),
        token_type: "refresh".to_owned(),
    };

    jsonwebtoken::encode(&Header::default(), &claims, &EncodingKey::from_secret(secret.as_ref()))
}

pub fn verify_jwt(token: &str, secret: &str) -> Result<Claims, Error> {
    let decoding_key = DecodingKey::from_secret(secret.as_ref());
    let validation = Validation::default();
    let token_data = jsonwebtoken::decode::<Claims>(token, &decoding_key, &validation)?;
    Ok(token_data.claims)
}

#[derive(Deserialize, ToSchema)]
struct Credentials {
    username: String,
    password: String,
}

#[derive(Serialize, ToSchema)]
struct AuthTokens {
    access_token: String,
    refresh_token: String,
    token_type: String,
}

#[derive(Deserialize, ToSchema)]
struct RefreshTokenRequest {
    refresh_token: String,
}

#[derive(Debug, Clone)]
struct CurrentUser {
    _id: String,
    role: String,
}

fn issue_tokens(username: &str, role: &str, secret: &str) -> Result<AuthTokens, StatusCode> {
    let access_token = create_jwt(username, role, secret).map_err(|_| StatusCode::INTERNAL_SERVER_ERROR)?;
    let refresh_token = create_refresh_jwt(username, role, secret).map_err(|_| StatusCode::INTERNAL_SERVER_ERROR)?;

    Ok(AuthTokens { access_token, refresh_token, token_type: "Bearer".to_owned() })
}

#[utoipa::path(
        post,
        path = "/login",
        tag = "Authentication",
        summary = "Login a user",
        description = "Authenticate a user with their username and password, returns access and refresh JWT tokens if successful.",
        request_body = Credentials,
        responses(
            (status = 200, description = "User authenticated successfully, returns access and refresh JWT tokens", body = AuthTokens),
            (status = 401, description = "Invalid username or password"),
            (status = 500, description = "Failed to authenticate user")
        )
    )]
async fn login(State(db): State<UsersDB>, Json(req): Json<Credentials>) -> impl IntoResponse {
    let user = db.get_user(&req.username, &req.password).await;
    match user {
        Ok(user) => issue_tokens(&user.username, &user.role, &db.secret).map(Json),
        Err(DatabaseError::NotFound(_)) => Err(StatusCode::UNAUTHORIZED),
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
            (status = 200, description = "User registered successfully, returns access and refresh JWT tokens", body = AuthTokens),
            (status = 409, description = "Username already exists"),
            (status = 500, description = "Failed to register user")
        )
    )]
async fn register(State(db): State<UsersDB>, Json(req): Json<Credentials>) -> impl IntoResponse {
    match db.create_user(&req.username, &req.password, "user").await {
        Ok(_) => match db.get_user(&req.username, &req.password).await {
            Ok(user) => issue_tokens(&user.username, &user.role, &db.secret).map(Json),
            Err(_) => Err(StatusCode::INTERNAL_SERVER_ERROR),
        },
        Err(DatabaseError::Exists(_)) => Err(StatusCode::CONFLICT),
        Err(_) => Err(StatusCode::INTERNAL_SERVER_ERROR),
    }
}

#[utoipa::path(
        post,
        path = "/refresh",
        tag = "Authentication",
        summary = "Refresh authentication tokens",
        description = "Exchange a valid refresh token for a new access and refresh JWT token pair.",
        request_body = RefreshTokenRequest,
        responses(
            (status = 200, description = "Tokens refreshed successfully", body = AuthTokens),
            (status = 401, description = "Invalid or expired refresh token"),
            (status = 500, description = "Failed to refresh tokens")
        )
    )]
async fn refresh_token(State(db): State<UsersDB>, Json(req): Json<RefreshTokenRequest>) -> impl IntoResponse {
    match verify_jwt(&req.refresh_token, &db.secret) {
        Ok(claims) if claims.token_type == "refresh" => issue_tokens(&claims.sub, &claims.role, &db.secret).map(Json).map_err(|_| StatusCode::INTERNAL_SERVER_ERROR),
        _ => Err(StatusCode::UNAUTHORIZED),
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
async fn get_users(State(db): State<UsersDB>, Extension(user): Extension<CurrentUser>) -> impl IntoResponse {
    if user.role != "admin" {
        return (StatusCode::FORBIDDEN, "Only admin users can view the list of users").into_response();
    }
    match db.get_users().await {
        Ok(users) => (StatusCode::OK, axum::Json(users)).into_response(),
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
async fn create_user(State(db): State<UsersDB>, Extension(user): Extension<CurrentUser>, Json(req): Json<Credentials>) -> impl IntoResponse {
    if user.role != "admin" {
        return (StatusCode::FORBIDDEN, "Only admin users can create new users").into_response();
    }
    match db.create_user(&req.username, &req.password, "user").await {
        Ok(_) => (StatusCode::CREATED, "User created successfully").into_response(),
        Err(DatabaseError::Exists(_)) => (StatusCode::CONFLICT, "Username already exists").into_response(),
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
async fn get_classes(State(coco): State<CoCo>) -> impl IntoResponse {
    trace!("Handling request to list all classes");
    match coco.get_classes().await {
        Ok(classes) => Json(classes).into_response(),
        Err(e) => (StatusCode::INTERNAL_SERVER_ERROR, format!("Failed to get classes: {}", e)).into_response(),
    }
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
async fn get_class(State(coco): State<CoCo>, Path(name): Path<String>) -> impl IntoResponse {
    trace!("Handling request to get class with name: {}", name);
    match coco.get_class(&name).await {
        Ok(Some(class)) => Json(class).into_response(),
        Ok(None) => (StatusCode::NOT_FOUND, format!("Class '{}' not found", name)).into_response(),
        Err(e) => (StatusCode::INTERNAL_SERVER_ERROR, format!("Failed to get class '{}': {}", name, e)).into_response(),
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
            (status = 409, description = "Class already exists"),
            (status = 500, description = "Failed to create class")
        )
    )]
async fn create_class(State(coco): State<CoCo>, Json(class): Json<Class>) -> impl IntoResponse {
    trace!("Handling request to create class with name: {}", class.name);
    match coco.create_class(class).await {
        Ok(_) => (StatusCode::CREATED, "Class created successfully".to_string()).into_response(),
        Err(e) => (StatusCode::INTERNAL_SERVER_ERROR, format!("Failed to create class: {}", e)).into_response(),
    }
}

#[derive(Debug, Deserialize, IntoParams)]
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
async fn get_objects(State(coco): State<CoCo>, Query(filter): Query<ObjectFilter>) -> impl IntoResponse {
    trace!("Handling request to list all objects with filter: {:?}", filter);
    match coco.get_objects().await {
        Ok(objects) => {
            let filtered_objects: Vec<OpenApiObject> = objects
                .into_iter()
                .filter(|o| {
                    let class_match = filter.class.as_ref().is_none_or(|class_name| o.classes.contains(class_name));
                    let extra_match = filter.extra.as_ref().is_none_or(|extra| extra.iter().all(|(k, v)| o.properties.as_ref().and_then(|props| props.get(k)).is_none_or(|prop| prop == v)));
                    class_match && extra_match
                })
                .collect();
            Json(filtered_objects).into_response()
        }
        Err(e) => (StatusCode::INTERNAL_SERVER_ERROR, format!("Failed to get objects: {}", e)).into_response(),
    }
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
async fn get_object(State(coco): State<CoCo>, Path(id): Path<String>) -> impl IntoResponse {
    trace!("Handling request to get object with ID: {}", id);
    match coco.get_object(&id).await {
        Ok(Some(object)) => Json(object).into_response(),
        Ok(None) => (StatusCode::NOT_FOUND, format!("Object with ID '{}' not found", id)).into_response(),
        Err(e) => (StatusCode::INTERNAL_SERVER_ERROR, format!("Failed to get object with ID '{}': {}", id, e)).into_response(),
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

struct SecurityAddon;

impl Modify for SecurityAddon {
    fn modify(&self, openapi: &mut utoipa::openapi::OpenApi) {
        let components = openapi.components.get_or_insert_with(Default::default);
        components.add_security_scheme("bearerAuth", SecurityScheme::Http(HttpBuilder::new().scheme(HttpAuthScheme::Bearer).bearer_format("JWT").build()));
    }
}

#[derive(OpenApi)]
#[openapi(
    servers(
        (url = "/", description = "Base URL for CoCo API")
    ),
    // paths(, get_objects, get_object, create_object, set_properties, add_data, get_data, get_rules, get_rule, create_rule, ws_handler, openapi),
    paths(get_users, create_user, register, login, refresh_token, get_classes, get_class, create_class),
    components(
        schemas(Class, Rule, Property, OpenApiObject, OpenApiValue, User, Credentials, AuthTokens, RefreshTokenRequest)
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

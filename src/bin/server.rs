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
use coco::{CLIPSKnowledgeBase, Class, CoCo, MongoDatabase, Notifier, Object};
use serde_json::Value;
use std::sync::Arc;
use tokio::sync::broadcast::Sender;
use tower_http::services::{ServeDir, ServeFile};
use utoipa::OpenApi;

struct ServerNotifier {
    tx: Sender<Value>,
}

impl ServerNotifier {
    fn new(tx: Sender<Value>) -> Self {
        Self { tx }
    }
}

struct AppState {
    notifier: Arc<ServerNotifier>,
    coco: CoCo<coco::MongoDatabase, coco::CLIPSKnowledgeBase>,
}

impl Notifier for ServerNotifier {
    fn class_created(&self, class: &Class) {
        println!("Notifier: class created: {}", class.name);
        let mut msg = serde_json::to_value(class).unwrap();
        msg.as_object_mut().unwrap().insert("msg_type".to_string(), serde_json::Value::String("class_created".to_string()));
        let _ = self.tx.send(msg);
    }

    fn object_created(&self, object: &Object) {
        println!("Notifier: object created: {}", object.id);
        let mut msg = serde_json::to_value(object).unwrap();
        msg.as_object_mut().unwrap().insert("msg_type".to_string(), serde_json::Value::String("object_created".to_string()));
        let _ = self.tx.send(msg);
    }
}

#[tokio::main]
async fn main() {
    let (tx, _rx) = tokio::sync::broadcast::channel(100);
    println!("Starting COCO server...");
    println!("Connecting to MongoDB at mongodb://localhost:27017, database: coco_server");
    let db = MongoDatabase::new("coco_server", "mongodb://localhost:27017").await.unwrap();
    println!("Initializing knowledge base...");
    let kb = CLIPSKnowledgeBase::new();
    let notifier = Arc::new(ServerNotifier::new(tx));
    let coco = CoCo::new(db, kb, Some(notifier.clone())).await;
    let app_state = Arc::new(AppState { notifier, coco });

    let app = Router::new();
    let app = app.route("/ws", get(ws_handler));
    let app = app.route("/classes", get(get_classes).post(create_class));
    let app = app.route("/classes/{name}", get(get_class));
    let app = app.route("/objects", get(get_objects).post(create_object));
    let app = app.route("/objects/{id}", get(get_object));
    let app = app.route("/openapi", get(openapi));
    let app = app.with_state(app_state).nest_service("/assets", ServeDir::new("gui/dist/assets")).fallback_service(ServeDir::new("gui/dist").not_found_service(ServeFile::new("gui/dist/index.html")));

    println!("Server running on http://0.0.0.0:3000");
    let listener = tokio::net::TcpListener::bind("0.0.0.0:3000").await.unwrap();
    axum::serve(listener, app).await.unwrap();
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
async fn get_classes(State(state): State<Arc<AppState>>) -> impl IntoResponse {
    axum::Json(state.coco.get_classes())
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
async fn get_class(Path(name): Path<String>, State(state): State<Arc<AppState>>) -> impl IntoResponse {
    match state.coco.get_class(&name) {
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
        )
    )]
async fn create_class(State(state): State<Arc<AppState>>, axum::Json(class): axum::Json<Class>) -> impl IntoResponse {
    state.coco.create_class(&class.name, class.parents, class.static_properties, class.dynamic_properties).await;
    StatusCode::CREATED
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
async fn get_objects(State(state): State<Arc<AppState>>) -> impl IntoResponse {
    axum::Json(state.coco.get_objects())
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
async fn get_object(Path(id): Path<String>, State(state): State<Arc<AppState>>) -> impl IntoResponse {
    match state.coco.get_object(&id) {
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
            (status = 201, description = "Object created successfully"),
        )
    )]
async fn create_object(State(state): State<Arc<AppState>>, axum::Json(object): axum::Json<Object>) -> impl IntoResponse {
    state.coco.create_object(object.classes, object.properties, object.values).await;
    StatusCode::CREATED
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
async fn ws_handler(ws: WebSocketUpgrade, State(state): State<Arc<AppState>>) -> impl IntoResponse {
    ws.on_upgrade(move |socket| handle_socket(socket, state))
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

async fn handle_socket(mut socket: WebSocket, state: Arc<AppState>) {
    let classes_map: std::collections::HashMap<String, serde_json::Value> = state
        .coco
        .get_classes()
        .into_iter()
        .map(|c| {
            let name = c.name.clone();
            let mut v = serde_json::to_value(c).unwrap();
            v.as_object_mut().unwrap().remove("name");
            (name, v)
        })
        .collect();
    let init_msg = serde_json::json!({
        "msg_type": "coco",
        "classes": classes_map
    });
    socket.send(Message::Text(serde_json::to_string(&init_msg).unwrap().into())).await.unwrap();

    let mut rx = state.notifier.tx.subscribe();
    while let Ok(msg) = rx.recv().await {
        println!("Sending WebSocket message: {:?}", msg);
        socket.send(Message::Text(serde_json::to_string(&msg).unwrap().into())).await.unwrap();
    }
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
struct ApiDoc;

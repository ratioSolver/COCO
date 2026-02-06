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
use coco::{CLIPSKnowledgeBase, Class, CoCo, MongoDBDataStore, Object};
use std::sync::{Arc, Mutex};
use tokio::sync::broadcast;
use tower_http::services::ServeDir;
use utoipa::OpenApi;

#[tokio::main]
async fn main() {
    let db = Arc::new(MongoDBDataStore::new("coco_server", "mongodb://localhost:27017").await.unwrap());
    let (tx, _rx) = broadcast::channel(100);
    let kb = Arc::new(Mutex::new(CLIPSKnowledgeBase::new(tx)));
    let coco = Arc::new(CoCo::new(db, kb).await);

    let app = Router::new();
    let app = app.route("/ws", get(ws_handler));
    let app = app.route("/classes", get(get_classes).post(create_class));
    let app = app.route("/classes/{name}", get(get_class));
    let app = app.route("/objects", get(get_objects).post(create_object));
    let app = app.route("/objects/{id}", get(get_object));
    let app = app.route("/openapi", get(openapi));
    let app = app.with_state(coco).nest_service("/assets", ServeDir::new("gui/dist/assets")).fallback_service(ServeDir::new("gui/dist").not_found_service(ServeFile::new("gui/dist/index.html")));

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
async fn get_classes(State(coco): State<Arc<CoCo>>) -> impl IntoResponse {
    axum::Json(coco.get_classes())
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
async fn get_class(Path(name): Path<String>, State(coco): State<Arc<CoCo>>) -> impl IntoResponse {
    match coco.get_class(&name) {
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
async fn create_class(State(coco): State<Arc<CoCo>>, axum::Json(class): axum::Json<Class>) -> impl IntoResponse {
    coco.create_class(&class).await;
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
async fn get_objects(State(coco): State<Arc<CoCo>>) -> impl IntoResponse {
    axum::Json(coco.get_objects())
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
async fn get_object(Path(id): Path<String>, State(coco): State<Arc<CoCo>>) -> impl IntoResponse {
    match coco.get_object(&id) {
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
async fn create_object(State(coco): State<Arc<CoCo>>, axum::Json(object): axum::Json<Object>) -> impl IntoResponse {
    coco.create_object(object.classes, object.properties, object.values).await;
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
async fn ws_handler(ws: WebSocketUpgrade, State(coco): State<Arc<CoCo>>) -> impl IntoResponse {
    ws.on_upgrade(move |socket| handle_socket(socket, coco))
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

async fn handle_socket(mut socket: WebSocket, coco: Arc<CoCo>) {
    let classes_map: std::collections::HashMap<String, serde_json::Value> = coco
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

    let mut rx = coco.lock().unwrap().get_event_sender().subscribe();
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

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
use coco::{CLIPSKnowledgeBase, CoCo, MongoDatabase};
use std::sync::Arc;
use tokio::sync::broadcast::Sender;
use tower_http::services::{ServeDir, ServeFile};

struct AppState {
    tx: Sender<String>,
    coco: CoCo<coco::MongoDatabase, coco::CLIPSKnowledgeBase>,
}

#[tokio::main]
async fn main() {
    let (tx, _rx) = tokio::sync::broadcast::channel(100);
    let coco = CoCo::new(MongoDatabase::new("coco_server", "mongodb://localhost:27017").await.unwrap(), CLIPSKnowledgeBase::new()).await;

    let app_state = Arc::new(AppState { tx, coco });

    let app = Router::new();
    let app = app.route("/ws", get(ws_handler));
    let app = app.route("/classes", get(get_classes));
    let app = app.route("/classes/:name", get(get_class)); // Added new route
    let app = app.with_state(app_state).nest_service("/assets", ServeDir::new("gui/dist/assets")).fallback_service(ServeDir::new("gui/dist").not_found_service(ServeFile::new("gui/dist/index.html")));

    let listener = tokio::net::TcpListener::bind("0.0.0.0:3000").await.unwrap();
    axum::serve(listener, app).await.unwrap();
}

async fn get_classes(State(state): State<Arc<AppState>>) -> impl IntoResponse {
    axum::Json(state.coco.get_classes())
}

async fn get_class(Path(name): Path<String>, State(state): State<Arc<AppState>>) -> impl IntoResponse {
    match state.coco.get_class(&name) {
        // We clone the single class to safely return it
        Some(class) => axum::Json(class).into_response(),
        None => (StatusCode::NOT_FOUND, "Class not found").into_response(),
    }
}

async fn ws_handler(ws: WebSocketUpgrade, State(state): State<Arc<AppState>>) -> impl IntoResponse {
    ws.on_upgrade(move |socket| handle_socket(socket, state))
}

async fn handle_socket(mut socket: WebSocket, state: Arc<AppState>) {
    let mut rx = state.tx.subscribe();
    while let Ok(msg) = rx.recv().await {
        if socket.send(Message::Text(msg.into())).await.is_err() {
            break;
        }
    }
}

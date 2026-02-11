use axum::Router;
use coco::CoCo;
use std::sync::Arc;
use tower_http::services::{ServeDir, ServeFile};

#[tokio::main]
async fn main() {
    let db = coco::db::mongodb::MongoDB::new("coco_db", "mongodb://localhost:27017").await.unwrap();
    let kb = coco::kb::clips::CLIPSKnowledgeBase::new();
    let coco = Arc::new(CoCo::new(db, kb).await);

    let app = Router::new();
    let app = app.with_state(coco).nest_service("/assets", ServeDir::new("gui/dist/assets")).fallback_service(ServeDir::new("gui/dist").not_found_service(ServeFile::new("gui/dist/index.html")));

    println!("Server running on http://0.0.0.0:3000");
    let listener = tokio::net::TcpListener::bind("0.0.0.0:3000").await.unwrap();
    axum::serve(listener, app).await.unwrap();
}

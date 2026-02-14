use coco::{
    CoCo,
    db::mongodb::MongoDB,
    kb::clips::CLIPSKnowledgeBase,
    llm::ollama::Ollama,
    server::{CoCoState, build_coco_router},
};
use std::sync::Arc;
use tower_http::services::{ServeDir, ServeFile};

#[derive(Clone)]
struct AppState {
    coco: Arc<CoCo>,
}

// 2. Implement the trait so the base router works
impl CoCoState for AppState {
    fn coco(&self) -> Arc<CoCo> {
        self.coco.clone()
    }
}

#[tokio::main]
async fn main() {
    let db = Arc::new(MongoDB::new("coco_db", "mongodb://localhost:27017").await.unwrap());
    let kb = Box::new(CLIPSKnowledgeBase::new());
    let llm = Box::new(Ollama::new("localhost".to_string(), 11434, "llama3".to_string()));
    let coco = Arc::new(CoCo::new(db, kb, Some(llm)).await);
    let state = AppState { coco };

    let app = build_coco_router::<AppState>();
    let app = app.with_state(state).nest_service("/assets", ServeDir::new("gui/dist/assets")).fallback_service(ServeDir::new("gui/dist").not_found_service(ServeFile::new("gui/dist/index.html")));

    println!("Server running on http://0.0.0.0:3000");
    let listener = tokio::net::TcpListener::bind("0.0.0.0:3000").await.unwrap();
    axum::serve(listener, app).await.unwrap();
}

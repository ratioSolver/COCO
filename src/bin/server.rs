use coco::{
    CoCo,
    db::mongodb::MongoDB,
    fcm::FCMClient,
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
    let llm = setup_llm();
    let fcm = setup_fcm();

    let coco = Arc::new(CoCo::new(db, kb, llm, fcm).await);
    let state = AppState { coco };

    let app = build_coco_router::<AppState>();
    let app = app.with_state(state).nest_service("/assets", ServeDir::new("gui/dist/assets")).fallback_service(ServeDir::new("gui/dist").not_found_service(ServeFile::new("gui/dist/index.html")));

    println!("Server running on http://0.0.0.0:3000");
    let listener = tokio::net::TcpListener::bind("0.0.0.0:3000").await.unwrap();
    axum::serve(listener, app).await.unwrap();
}

fn setup_llm() -> Option<Box<dyn coco::llm::LLM>> {
    #[cfg(feature = "ollama")]
    return Some(Box::new(Ollama::new("localhost", 11434, "llama3")));

    #[cfg(not(feature = "ollama"))]
    None
}

fn setup_fcm() -> Option<FCMClient> {
    #[cfg(feature = "fcm")]
    return Some(FCMClient::new("coco-project-id".to_string()));

    #[cfg(not(feature = "fcm"))]
    None
}

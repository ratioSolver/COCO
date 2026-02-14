#[cfg(feature = "ollama")]
use coco::llm::ollama::Ollama;
#[cfg(feature = "fcm")]
use coco::msg::fcm::FCMClient;
#[cfg(feature = "server")]
use coco::server::build_coco_router;
use coco::{CoCo, CoCoState, db::mongodb::MongoDB, kb::clips::CLIPSKnowledgeBase, msg::Messaging};
use std::sync::Arc;
#[cfg(feature = "server")]
use tower_http::services::{ServeDir, ServeFile};

#[derive(Clone)]
struct AppState {
    coco: Arc<CoCo>,
}

#[cfg(feature = "server")]
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
    let msg = setup_messaging();

    let coco = Arc::new(CoCo::new(db, kb, llm, msg).await);
    let state = AppState { coco };

    #[cfg(feature = "server")]
    {
        let app = build_coco_router::<AppState>();
        let app = app.with_state(state).nest_service("/assets", ServeDir::new("gui/dist/assets")).fallback_service(ServeDir::new("gui/dist").not_found_service(ServeFile::new("gui/dist/index.html")));

        println!("Server running on http://0.0.0.0:3000");
        let listener = tokio::net::TcpListener::bind("0.0.0.0:3000").await.unwrap();
        axum::serve(listener, app).await.unwrap();
    }
}

fn setup_llm() -> Option<Box<dyn coco::llm::LLM>> {
    #[cfg(feature = "ollama")]
    return Some(Box::new(Ollama::new("localhost", 11434, "llama3")));

    #[cfg(not(feature = "ollama"))]
    None
}

fn setup_messaging() -> Option<Box<dyn Messaging>> {
    #[cfg(feature = "fcm")]
    return Some(Box::new(FCMClient::new("coco-project-id".to_string())));

    #[cfg(not(feature = "fcm"))]
    None
}

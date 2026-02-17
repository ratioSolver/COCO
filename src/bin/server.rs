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
    let db = setup_db().await;
    let kb = setup_kb();
    let llm = setup_llm();
    let msg = setup_messaging();

    let coco = Arc::new(CoCo::new(db, kb, llm, msg).await);
    let state = AppState { coco };

    #[cfg(feature = "server")]
    {
        let app = build_coco_router::<AppState>();
        let app = app.with_state(state).nest_service("/assets", ServeDir::new("gui/dist/assets")).fallback_service(ServeDir::new("gui/dist").not_found_service(ServeFile::new("gui/dist/index.html")));

        let port = std::env::var("PORT").ok().and_then(|p| p.parse().ok()).unwrap_or(3000);

        println!("Starting server on port {}", port);
        let listener = tokio::net::TcpListener::bind(format!("0.0.0.0:{}", port)).await.unwrap();
        axum::serve(listener, app).await.unwrap();
    }
}

async fn setup_db() -> Arc<dyn coco::db::Database> {
    #[cfg(feature = "mongodb")]
    return setup_mongodb().await;

    #[cfg(not(feature = "mongodb"))]
    panic!("No database backend configured");
}

#[cfg(feature = "mongodb")]
async fn setup_mongodb() -> Arc<dyn coco::db::Database> {
    let name = std::env::var("DB_NAME").unwrap_or_else(|_| "coco_db".to_string());
    let host = std::env::var("DB_HOST").unwrap_or_else(|_| "localhost".to_string());
    let port = std::env::var("DB_PORT").ok().and_then(|p| p.parse().ok()).unwrap_or(27017);
    let uri = format!("mongodb://{}:{}", host, port);
    Arc::new(MongoDB::new(&name, &uri).await.unwrap())
}

fn setup_kb() -> Box<dyn coco::kb::KnowledgeBase> {
    #[cfg(feature = "clips")]
    return setup_clips();

    #[cfg(not(feature = "clips"))]
    panic!("No knowledge base backend configured");
}

#[cfg(feature = "clips")]
fn setup_clips() -> Box<dyn coco::kb::KnowledgeBase> {
    Box::new(CLIPSKnowledgeBase::new())
}

fn setup_llm() -> Option<Box<dyn coco::llm::LLM>> {
    #[cfg(feature = "ollama")]
    return Some(setup_ollama());

    #[cfg(not(feature = "ollama"))]
    None
}

#[cfg(feature = "ollama")]
fn setup_ollama() -> Box<dyn coco::llm::LLM> {
    let host = std::env::var("LLM_HOST").unwrap_or_else(|_| "localhost".to_string());
    let port = std::env::var("LLM_PORT").ok().and_then(|p| p.parse().ok()).unwrap_or(11434);
    let model = std::env::var("LLM_MODEL").unwrap_or_else(|_| "llama3".to_string());
    Box::new(Ollama::new(host, port, model))
}

fn setup_messaging() -> Option<Box<dyn Messaging>> {
    #[cfg(feature = "fcm")]
    return Some(setup_fcm());

    #[cfg(not(feature = "fcm"))]
    None
}

#[cfg(feature = "fcm")]
fn setup_fcm() -> Box<dyn Messaging> {
    let project_id = std::env::var("FCM_PROJECT_ID").unwrap_or_else(|_| "coco-project-id".to_string());
    Box::new(FCMClient::new(project_id))
}

#[cfg(feature = "server")]
use coco::server::build_coco_router;
use coco::{CoCo, CoCoState, db::setup_db, kb::setup_kb, llm::setup_llm, msg::setup_messaging};
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
    let coco = Arc::new(CoCo::new(setup_db().await, setup_kb(), setup_llm(), setup_messaging()).await);
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

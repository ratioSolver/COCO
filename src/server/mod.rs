use crate::{CoCo, kb::KnowledgeBase};
use std::sync::Arc;
use tokio::sync::RwLock;
#[cfg(not(feature = "secure"))]
use tracing::info;

#[cfg(feature = "secure")]
pub mod secure;
#[cfg(not(feature = "secure"))]
pub mod unsecure;

pub async fn start_server<KB>(coco: Arc<RwLock<CoCo<KB>>>)
where
    KB: KnowledgeBase + 'static,
{
    #[cfg(feature = "secure")]
    start_secure_server(coco).await;

    #[cfg(not(feature = "secure"))]
    start_unsecure_server(coco).await;
}

#[cfg(feature = "secure")]
async fn start_secure_server<KB>(coco: Arc<RwLock<CoCo<KB>>>)
where
    KB: KnowledgeBase + 'static,
{
    use crate::server::secure::{SecureCoCoState, secure_server::build_coco_router, setup_db};
    use tower_http::services::{ServeDir, ServeFile};

    let state = SecureCoCoState::new(coco, setup_db().await).await;
    let app = build_coco_router::<SecureCoCoState<KB>, KB>(state.clone());
    let app = app.with_state(state).nest_service("/assets", ServeDir::new("gui/dist/assets")).fallback_service(ServeDir::new("gui/dist").not_found_service(ServeFile::new("gui/dist/index.html")));

    let port = std::env::var("PORT").ok().and_then(|p| p.parse().ok()).unwrap_or(3000);
    println!("Starting CoCo server on port {}", port);
    let listener = tokio::net::TcpListener::bind(format!("0.0.0.0:{}", port)).await.unwrap();
    axum::serve(listener, app).await.unwrap();
}

#[cfg(not(feature = "secure"))]
async fn start_unsecure_server<KB>(coco: Arc<RwLock<CoCo<KB>>>)
where
    KB: KnowledgeBase + 'static,
{
    use crate::server::unsecure::{UnsecureCoCoState, build_coco_router};
    use tower_http::services::{ServeDir, ServeFile};

    let state = UnsecureCoCoState::new(coco).await;
    let app = build_coco_router::<UnsecureCoCoState<KB>, KB>();
    let app = app.with_state(state).nest_service("/assets", ServeDir::new("gui/dist/assets")).fallback_service(ServeDir::new("gui/dist").not_found_service(ServeFile::new("gui/dist/index.html")));

    let port = std::env::var("PORT").ok().and_then(|p| p.parse().ok()).unwrap_or(3000);
    info!("Starting CoCo server on port {}", port);
    let listener = tokio::net::TcpListener::bind(format!("0.0.0.0:{}", port)).await.unwrap();
    axum::serve(listener, app).await.unwrap();
}

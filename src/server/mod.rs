use std::sync::Arc;

use crate::CoCo;

#[cfg(feature = "secure")]
pub mod secure;
#[cfg(not(feature = "secure"))]
pub mod unsecure;

pub async fn start_server(coco: Arc<CoCo>) {
    #[cfg(feature = "secure")]
    start_secure_server(coco).await;

    #[cfg(not(feature = "secure"))]
    start_unsecure_server(coco).await;
}

#[cfg(feature = "secure")]
async fn start_secure_server(coco: Arc<CoCo>) {
    use crate::main::AppState;
    use crate::server::auth::{SecureCoCoState, build_secure_coco_router};
    use tower_http::services::{ServeDir, ServeFile};

    let coco = AppState { coco: crate::CoCo::default().await };
    let state = MongoSecureCoCoState::new(coco.coco.clone(), "coco_db", "mongodb://localhost:27017").await.expect("Failed to initialize MongoDB state");
    let app = build_secure_coco_router::<MongoSecureCoCoState>(state.clone()).with_state(state).nest_service("/assets", ServeDir::new("gui/dist/assets")).fallback_service(ServeDir::new("gui/dist").not_found_service(ServeFile::new("gui/dist/index.html")));

    let port = std::env::var("PORT").ok().and_then(|p| p.parse().ok()).unwrap_or(3000);
    println!("Starting CoCo server on port {}", port);
    let listener = tokio::net::TcpListener::bind(format!("0.0.0.0:{}", port)).await.unwrap();
    axum::serve(listener, app).await.unwrap();
}

#[cfg(not(feature = "secure"))]
async fn start_unsecure_server(coco: Arc<CoCo>) {
    use crate::server::unsecure::{UnsecureCoCoState, build_coco_router};
    use tower_http::services::{ServeDir, ServeFile};

    let state = UnsecureCoCoState::new(coco);
    let app = build_coco_router::<UnsecureCoCoState>();
    let app = app.with_state(state).nest_service("/assets", ServeDir::new("gui/dist/assets")).fallback_service(ServeDir::new("gui/dist").not_found_service(ServeFile::new("gui/dist/index.html")));

    let port = std::env::var("PORT").ok().and_then(|p| p.parse().ok()).unwrap_or(3000);
    println!("Starting CoCo server on port {}", port);
    let listener = tokio::net::TcpListener::bind(format!("0.0.0.0:{}", port)).await.unwrap();
    axum::serve(listener, app).await.unwrap();
}

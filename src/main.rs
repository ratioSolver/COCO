use coco::{CoCo, CoCoState};
use std::sync::Arc;
use tracing::{Level, info};
use tracing_subscriber;

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
    let subscriber = tracing_subscriber::fmt().with_max_level(Level::TRACE).finish();
    tracing::subscriber::set_global_default(subscriber).expect("Failed to set global default subscriber");
    let coco = Arc::new(CoCo::default().await);
    let state = AppState { coco };

    #[cfg(feature = "mqtt")]
    {
        use coco::mqtt::start_mqtt;

        let mqtt_broker = std::env::var("MQTT_BROKER").unwrap_or_else(|_| "localhost".to_string());
        let mqtt_port = std::env::var("MQTT_PORT").ok().and_then(|p| p.parse().ok()).unwrap_or(1883);
        start_mqtt(state.coco.clone(), mqtt_broker, mqtt_port).await;
    }

    #[cfg(feature = "server")]
    {
        use coco::server::build_coco_router;
        use tower_http::services::{ServeDir, ServeFile};

        let app = build_coco_router::<AppState>();
        let app = app.with_state(state).nest_service("/assets", ServeDir::new("gui/dist/assets")).fallback_service(ServeDir::new("gui/dist").not_found_service(ServeFile::new("gui/dist/index.html")));

        let port = std::env::var("PORT").ok().and_then(|p| p.parse().ok()).unwrap_or(3000);

        info!("Starting CoCo server on port {}", port);
        let listener = tokio::net::TcpListener::bind(format!("0.0.0.0:{}", port)).await.unwrap();
        axum::serve(listener, app).await.unwrap();
    }
}

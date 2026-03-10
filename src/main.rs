use coco::CoCo;
#[cfg(feature = "server")]
use coco::server::start_server;
use std::sync::Arc;
use tracing::Level;
use tracing_subscriber;

#[tokio::main]
async fn main() {
    let subscriber = tracing_subscriber::fmt().with_max_level(Level::TRACE).finish();
    tracing::subscriber::set_global_default(subscriber).expect("Failed to set global default subscriber");
    let coco = Arc::new(CoCo::default().await);

    #[cfg(feature = "mqtt")]
    {
        use coco::mqtt::start_mqtt;

        let mqtt_broker = std::env::var("MQTT_BROKER").unwrap_or_else(|_| "localhost".to_string());
        let mqtt_port = std::env::var("MQTT_PORT").ok().and_then(|p| p.parse().ok()).unwrap_or(1883);
        start_mqtt(coco.clone(), mqtt_broker, mqtt_port).await;
    }

    #[cfg(feature = "server")]
    start_server(coco).await;
}

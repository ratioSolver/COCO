use coco::CoCo;
#[cfg(feature = "mqtt")]
use coco::mqtt::start_mqtt;
#[cfg(feature = "server")]
use coco::server::start_server;
use std::sync::Arc;
use tracing::Level;

#[tokio::main]
async fn main() {
    let subscriber = tracing_subscriber::fmt().with_max_level(Level::TRACE).finish();
    tracing::subscriber::set_global_default(subscriber).expect("Failed to set global default subscriber");
    let coco = Arc::new(CoCo::default().await);

    #[cfg(feature = "mqtt")]
    start_mqtt(coco.clone()).await;

    #[cfg(feature = "server")]
    start_server(coco).await;
}

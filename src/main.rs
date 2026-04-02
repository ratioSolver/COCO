#[cfg(feature = "fcm")]
use coco::fcm::setup_fcm;
#[cfg(feature = "ollama")]
use coco::kb::clips::ollama::setup_ollama;
#[cfg(feature = "server")]
use coco::server::start_server;
use coco::{CoCo, db::setup_mongodb, kb::setup_clips};
use tracing::{Level, error, subscriber};

#[tokio::main]
async fn main() {
    let subscriber = tracing_subscriber::fmt().with_max_level(Level::TRACE).finish();
    subscriber::set_global_default(subscriber).expect("Failed to set global default subscriber");

    let db = setup_mongodb().await.unwrap_or_else(|e| {
        error!("Failed to set up MongoDB: {}", e);
        std::process::exit(1);
    });

    let kb = setup_clips().unwrap_or_else(|e| {
        error!("Failed to set up knowledge base: {}", e);
        std::process::exit(1);
    });

    #[cfg(feature = "ollama")]
    setup_ollama(&kb).unwrap_or_else(|e| {
        error!("Failed to set up Ollama integration: {}", e);
        std::process::exit(1);
    });

    #[cfg(feature = "fcm")]
    let fcm_router = setup_fcm(&db, &kb).unwrap_or_else(|e| {
        error!("Failed to add FCM to knowledge base: {}", e);
        std::process::exit(1);
    });

    let coco = CoCo::new(db, kb).await;

    #[cfg(feature = "server")]
    start_server(coco).await;
}

#[cfg(feature = "ollama")]
use coco::kb::clips::ollama::add_ollama;
#[cfg(feature = "server")]
use coco::server::start_server;
use coco::{CoCo, db::setup_db, kb::setup_clips};
use tracing::{Level, error, subscriber};

#[tokio::main]
async fn main() {
    let subscriber = tracing_subscriber::fmt().with_max_level(Level::TRACE).finish();
    subscriber::set_global_default(subscriber).expect("Failed to set global default subscriber");

    let db = setup_db().await.unwrap_or_else(|e| {
        error!("Failed to set up database: {}", e);
        std::process::exit(1);
    });

    let kb = setup_clips().unwrap_or_else(|e| {
        error!("Failed to set up knowledge base: {}", e);
        std::process::exit(1);
    });

    #[cfg(feature = "ollama")]
    add_ollama(&kb, "localhost".to_string(), 11434, "coco-ollama".to_string()).unwrap_or_else(|e| {
        error!("Failed to add Ollama to knowledge base: {}", e);
        std::process::exit(1);
    });

    let coco = CoCo::new(db, kb).await;

    #[cfg(feature = "server")]
    start_server(coco).await;
}

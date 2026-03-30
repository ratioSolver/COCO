#[cfg(feature = "server")]
use coco::server::start_server;
use coco::{CoCo, db::setup_db, kb::setup_kb, llm::setup_llm, msg::setup_messaging};
use std::sync::Arc;
use tokio::sync::RwLock;
use tracing::{Level, error, subscriber};

#[tokio::main]
async fn main() {
    let subscriber = tracing_subscriber::fmt().with_max_level(Level::TRACE).finish();
    subscriber::set_global_default(subscriber).expect("Failed to set global default subscriber");

    let kb = match setup_kb() {
        Ok(kb) => kb,
        Err(e) => {
            error!("Failed to set up knowledge base: {}", e);
            return;
        }
    };
    let db = match setup_db().await {
        Ok(db) => db,
        Err(e) => {
            error!("Failed to set up database: {}", e);
            return;
        }
    };
    let llm = setup_llm();
    let msg = setup_messaging();

    let coco = Arc::new(RwLock::new(CoCo::new(kb, db, llm, msg).await));

    #[cfg(feature = "server")]
    start_server(coco).await;
}

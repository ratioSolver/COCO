#[cfg(feature = "server")]
use coco::server::start_server;
use coco::{
    CoCo,
    db::setup_db,
    kb::{self, clips::CLIPSKnowledgeBase, setup_kb},
};
use tracing::{Level, error, info, subscriber, trace};

#[tokio::main]
async fn main() {
    let subscriber = tracing_subscriber::fmt().with_max_level(Level::TRACE).finish();
    subscriber::set_global_default(subscriber).expect("Failed to set global default subscriber");

    let db = match setup_db().await {
        Ok(db) => db,
        Err(e) => {
            error!("Failed to set up database: {}", e);
            return;
        }
    };

    let kb = CLIPSKnowledgeBase::new();

    let coco = CoCo::new(db, kb).await;

    #[cfg(feature = "server")]
    start_server(coco).await;
}

use crate::{db::Database, kb::KnowledgeBase, model::CoCoEvent};
use tokio::sync::broadcast;

pub mod db;
pub mod kb;
pub mod model;
#[cfg(feature = "server")]
pub mod server;

#[derive(Clone)]
pub struct CoCo<DB, KB>
where
    DB: Database,
    KB: KnowledgeBase,
{
    pub db: DB,
    pub kb: KB,
    pub event_tx: broadcast::Sender<CoCoEvent>,
}
impl<DB, KB> CoCo<DB, KB>
where
    DB: Database,
    KB: KnowledgeBase,
{
    pub async fn new(db: DB, mut kb: KB) -> Self {
        let (event_tx, _) = broadcast::channel(100);

        if let Some(mut event_rx) = kb.take_event_receiver() {
            let db_clone = db.clone();

            // Task di Tokio per processare gli eventi in background
            tokio::spawn(async move {
                while let Some(event) = event_rx.recv().await {
                    match event {
                        _ => {}
                    }
                }
            });
        }

        Self { db, kb, event_tx }
    }
}

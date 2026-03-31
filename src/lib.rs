use crate::{db::Database, kb::KnowledgeBase, model::CoCoEvent};
use tokio::sync::broadcast;

pub mod db;
pub mod kb;
pub mod model;

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

use crate::{DataStore, KnowledgeBase};
use futures::lock::Mutex;
use std::sync::Arc;
use tokio::sync::broadcast;

pub enum CoCoEvent {
    ClassCreated(String),
    ObjectCreated(String),
}

pub struct CoCo {
    tx: broadcast::Sender<CoCoEvent>,
    db: Arc<dyn DataStore>,
    kb: Arc<Mutex<dyn KnowledgeBase>>,
}

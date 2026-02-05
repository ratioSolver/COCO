use crate::{DataStore, KnowledgeBase};
use std::sync::{Arc, Mutex};
use tokio::sync::broadcast;

#[derive(Clone)]
pub enum CoCoEvent {
    ClassCreated(String),
    ObjectCreated(String),
    AddedValues(String, String, Vec<String>),
}

pub struct CoCo {
    tx: broadcast::Sender<CoCoEvent>,
    db: Arc<dyn DataStore>,
    kb: Arc<Mutex<dyn KnowledgeBase>>,
}

impl CoCo {
    pub async fn new(db: Arc<dyn DataStore>, kb: Arc<Mutex<dyn KnowledgeBase>>) -> Self {
        let (tx, _rx) = broadcast::channel(100);
        kb.lock().unwrap().register_callback(tx.clone());
        Self { tx, db, kb }
    }
}

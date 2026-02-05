use crate::{DataStore, KnowledgeBase};
use futures::lock::Mutex;
use std::sync::Arc;

pub enum CoCoEvent {
    ClassCreated(String),
    ObjectCreated(String),
    AddedValues(String, String, Vec<String>),
}

pub struct CoCo {
    db: Arc<dyn DataStore>,
    kb: Arc<Mutex<dyn KnowledgeBase>>,
}

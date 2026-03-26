use crate::model::Value;
use chrono::{DateTime, Utc};
use std::collections::HashMap;

#[cfg(feature = "clips")]
mod clips;

#[derive(Debug)]
pub enum KnowledgeBaseError {
    CreationError(String),
    ClassAlreadyExists(String),
    ClassNotFound(String),
    ObjectAlreadyExists(String),
    ObjectNotFound(String),
    RuleAlreadyExists(String),
    RuleNotFound(String),
    KBError(String),
}

#[derive(Debug)]
pub enum KnowledgeBaseEvent {
    AddedClass(String, String),                                 // (object_id, class_name)
    UpdatedProperties(String, HashMap<String, Value>),          // (object_id, properties)
    AddedValues(String, HashMap<String, Value>, DateTime<Utc>), // (object_id, value, date_time)
}

pub trait KnowledgeBase {
    fn run(&mut self) -> Result<(), KnowledgeBaseError>;

    fn set_callback(&self, cb: impl Fn(KnowledgeBaseEvent) + 'static);
}

pub fn setup_kb() -> Result<impl KnowledgeBase, KnowledgeBaseError> {
    #[cfg(feature = "clips")]
    return setup_clips();

    #[cfg(not(feature = "clips"))]
    panic!("No knowledge base backend configured");
}

#[cfg(feature = "clips")]
fn setup_clips() -> Result<impl KnowledgeBase, KnowledgeBaseError> {
    use crate::kb::clips::CLIPSKnowledgeBase;

    CLIPSKnowledgeBase::new()
}

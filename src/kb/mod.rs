use std::collections::HashMap;

use chrono::{DateTime, Utc};
use tokio::sync::mpsc;

use crate::model::{Class, CoCoEvent, Object, Rule, Value};

#[cfg(feature = "clips")]
mod clips;

#[derive(Debug)]
pub enum KnowledgeBaseError {
    ClassAlreadyExists(String),
    ClassNotFound(String),
    ObjectAlreadyExists(String),
    ObjectNotFound(String),
    RuleAlreadyExists(String),
    RuleNotFound(String),
    KBError(String),
}

pub trait KnowledgeBase: Send + Sync {
    fn get_classes(&self) -> Vec<&Class>;
    fn get_class(&self, name: &str) -> Option<&Class>;
    fn create_class(&mut self, class: Class) -> Result<(), KnowledgeBaseError>;

    fn get_objects(&self) -> Vec<&Object>;
    fn get_object(&self, id: &str) -> Option<&Object>;
    fn create_object(&mut self, object: Object) -> Result<(), KnowledgeBaseError>;
    fn add_class(&mut self, object_id: &str, class_name: &str) -> Result<(), KnowledgeBaseError>;
    fn set_properties(&mut self, object_id: &str, properties: HashMap<String, Value>) -> Result<(), KnowledgeBaseError>;
    fn add_values(&mut self, object_id: &str, values: HashMap<String, Value>, date_time: DateTime<Utc>) -> Result<(), KnowledgeBaseError>;

    fn get_rules(&self) -> Vec<&Rule>;
    fn get_rule(&self, name: &str) -> Option<&Rule>;
    fn create_rule(&mut self, rule: Rule) -> Result<(), KnowledgeBaseError>;

    fn run(&mut self) -> Result<(), KnowledgeBaseError>;
}

pub fn setup_kb() -> (Box<dyn KnowledgeBase>, mpsc::UnboundedReceiver<CoCoEvent>) {
    #[cfg(feature = "clips")]
    return setup_clips();

    #[cfg(not(feature = "clips"))]
    panic!("No knowledge base backend configured");
}

#[cfg(feature = "clips")]
fn setup_clips() -> (Box<dyn KnowledgeBase>, mpsc::UnboundedReceiver<CoCoEvent>) {
    use crate::kb::clips::CLIPSKnowledgeBase;

    CLIPSKnowledgeBase::new()
}

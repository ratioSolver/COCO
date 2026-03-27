use crate::model::{Class, Object, Rule, Value};
use chrono::{DateTime, Utc};
use std::{collections::HashMap, fmt};

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

impl fmt::Display for KnowledgeBaseError {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        match self {
            KnowledgeBaseError::CreationError(msg) => write!(f, "Creation error: {}", msg),
            KnowledgeBaseError::ClassAlreadyExists(name) => write!(f, "Class already exists: {}", name),
            KnowledgeBaseError::ClassNotFound(name) => write!(f, "Class not found: {}", name),
            KnowledgeBaseError::ObjectAlreadyExists(id) => write!(f, "Object already exists: {}", id),
            KnowledgeBaseError::ObjectNotFound(id) => write!(f, "Object not found: {}", id),
            KnowledgeBaseError::RuleAlreadyExists(name) => write!(f, "Rule already exists: {}", name),
            KnowledgeBaseError::RuleNotFound(name) => write!(f, "Rule not found: {}", name),
            KnowledgeBaseError::KBError(msg) => write!(f, "Knowledge base error: {}", msg),
        }
    }
}

#[derive(Debug)]
pub enum KnowledgeBaseEvent {
    AddedClass(String, String),                                 // (object_id, class_name)
    UpdatedProperties(String, HashMap<String, Value>),          // (object_id, properties)
    AddedValues(String, HashMap<String, Value>, DateTime<Utc>), // (object_id, value, date_time)
    LLMPrompt(String, String),                                  // (object_id, prompt)
    AsyncLLMPrompt(String, String),                             // (object_id, prompt)
    Message(String, String, String),                            // (object_id, title, message)
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

    fn set_llm_result(&mut self, object_id: &str, result: &str) -> Result<(), KnowledgeBaseError>;

    fn run(&mut self) -> Result<(), KnowledgeBaseError>;

    fn set_callback(&self, cb: impl Fn(KnowledgeBaseEvent) + Send + Sync + 'static);
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

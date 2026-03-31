use crate::model::{Class, Object, Rule, Value};
use async_trait::async_trait;
use chrono::{DateTime, Utc};
use std::{collections::HashMap, fmt};
use tokio::sync::{mpsc, oneshot};

#[cfg(feature = "clips")]
mod clips;

#[derive(Debug)]
pub enum KBCommand {
    GetClasses(oneshot::Sender<Result<Vec<Class>, KnowledgeBaseError>>),
    GetClass(String, oneshot::Sender<Result<Class, KnowledgeBaseError>>),
    CreateClass(Class, oneshot::Sender<Result<(), KnowledgeBaseError>>),
    CreateObject(Object, oneshot::Sender<Result<(), KnowledgeBaseError>>),
    AddClass(String, String, oneshot::Sender<Result<(), KnowledgeBaseError>>),
    SetProperties(String, HashMap<String, Value>, oneshot::Sender<Result<(), KnowledgeBaseError>>),
    AddValues(String, HashMap<String, Value>, DateTime<Utc>, oneshot::Sender<Result<(), KnowledgeBaseError>>),
    CreateRule(Rule, oneshot::Sender<Result<(), KnowledgeBaseError>>),
    SetLLMResult(String, String, oneshot::Sender<Result<(), KnowledgeBaseError>>),
    Run(oneshot::Sender<Result<(), KnowledgeBaseError>>),
}

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
    Message(String, String, String),                            // (object_id, title, message)
}

impl fmt::Display for KnowledgeBaseEvent {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        match self {
            KnowledgeBaseEvent::AddedClass(object_id, class_name) => write!(f, "Added class '{}' to object '{}'", class_name, object_id),
            KnowledgeBaseEvent::UpdatedProperties(object_id, properties) => write!(f, "Updated properties for object '{}': {:?}", object_id, properties),
            KnowledgeBaseEvent::AddedValues(object_id, values, date_time) => write!(f, "Added values for object '{}': {:?} at {}", object_id, values, date_time),
            KnowledgeBaseEvent::LLMPrompt(object_id, prompt) => write!(f, "LLM prompt for object '{}': {}", object_id, prompt),
            KnowledgeBaseEvent::Message(object_id, title, message) => write!(f, "Message for object '{}': {} - {}", object_id, title, message),
        }
    }
}

#[async_trait]
pub trait KnowledgeBase: Clone + Send + Sync + 'static {
    async fn send_command(&self, cmd: KBCommand) -> Result<(), KnowledgeBaseError>;

    fn take_event_receiver(&mut self) -> Option<mpsc::Receiver<KnowledgeBaseEvent>>;
}

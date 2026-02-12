use std::collections::HashMap;

use chrono::{DateTime, Utc};

use crate::model::{Class, Object, Value};

#[cfg(feature = "clips")]
pub mod clips;

#[derive(Debug)]
pub enum KnowledgeBaseError {
    ClassAlreadyExists(String),
    ClassNotFound(String),
    ObjectAlreadyExists(String),
    ObjectNotFound(String),
    KBError(String),
}

pub trait KnowledgeBase {
    fn get_classes(&self) -> Vec<&Class>;
    fn get_class(&self, name: &str) -> Option<&Class>;
    fn create_class(&mut self, class: &Class) -> Result<(), KnowledgeBaseError>;

    fn get_objects(&self) -> Vec<&Object>;
    fn get_object(&self, id: &str) -> Option<&Object>;
    fn create_object(&mut self, object: &Object) -> Result<(), KnowledgeBaseError>;
    fn add_class(&mut self, object: &Object, class: &Class) -> Result<(), KnowledgeBaseError>;
    fn set_properties(&mut self, object: &Object, properties: HashMap<String, Value>) -> Result<(), KnowledgeBaseError>;
    fn add_values(&mut self, object: &Object, values: HashMap<String, Value>, date_time: DateTime<Utc>) -> Result<(), KnowledgeBaseError>;
}

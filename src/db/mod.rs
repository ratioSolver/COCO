use crate::model::Class;
use async_trait::async_trait;
use std::fmt;

#[cfg(feature = "mongodb")]
mod mongodb;

#[derive(Debug)]
pub enum DatabaseError {
    ConnectionError(String),
    ClassNotFound(String),
    ClassAlreadyExists(String),
    ObjectNotFound(String),
    ObjectAlreadyExists(String),
    RuleNotFound(String),
    RuleAlreadyExists(String),
}

impl fmt::Display for DatabaseError {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        match self {
            DatabaseError::ConnectionError(msg) => write!(f, "Connection error: {}", msg),
            DatabaseError::ClassNotFound(name) => write!(f, "Class not found: {}", name),
            DatabaseError::ClassAlreadyExists(name) => write!(f, "Class already exists: {}", name),
            DatabaseError::ObjectNotFound(id) => write!(f, "Object not found: {}", id),
            DatabaseError::ObjectAlreadyExists(id) => write!(f, "Object already exists: {}", id),
            DatabaseError::RuleNotFound(name) => write!(f, "Rule not found: {}", name),
            DatabaseError::RuleAlreadyExists(name) => write!(f, "Rule already exists: {}", name),
        }
    }
}

// Il Database deve essere asincrono e clonabile
#[async_trait]
pub trait Database: Clone + Send + Sync + 'static {
    fn name(&self) -> &str;

    async fn get_classes(&self) -> Result<Vec<Class>, DatabaseError>;
    async fn create_class(&self, class: Class) -> Result<(), DatabaseError>;
}

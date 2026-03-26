use crate::model::Class;
use async_trait::async_trait;
use std::fmt;

#[cfg(feature = "mongodb")]
mod mongo;

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

#[async_trait]
pub trait Database: Send + Sync {
    fn name(&self) -> &str;

    async fn create_class(&self, class: &Class) -> Result<(), DatabaseError>;

    async fn add_class(&self, object_id: &str, class_name: &str) -> Result<(), DatabaseError>;

    async fn drop_database(&self) -> Result<(), DatabaseError>;
}

pub async fn setup_db() -> Result<impl Database, DatabaseError> {
    #[cfg(feature = "mongodb")]
    return setup_mongodb().await;

    #[cfg(not(feature = "mongodb"))]
    panic!("No database backend configured");
}

#[cfg(feature = "mongodb")]
async fn setup_mongodb() -> Result<impl Database, DatabaseError> {
    use crate::db::mongo::MongoDB;

    let name = std::env::var("DB_NAME").unwrap_or_else(|_| "coco_db".to_owned());
    let host = std::env::var("DB_HOST").unwrap_or_else(|_| "localhost".to_owned());
    let port = std::env::var("DB_PORT").ok().and_then(|p| p.parse().ok()).unwrap_or(27017);
    let uri = format!("mongodb://{}:{}", host, port);
    MongoDB::new(&name, &uri).await
}

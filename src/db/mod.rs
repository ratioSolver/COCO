use crate::model::Class;
use async_trait::async_trait;

#[cfg(feature = "mongodb")]
pub mod mongodb;

#[derive(Debug)]
pub enum DatabaseError {
    ConnectionError(String),
    ClassNotFound(String),
    ClassAlreadyExists(String),
}

#[async_trait]
pub trait Database {
    fn name(&self) -> &str;

    async fn get_classes(&self) -> Result<Vec<Class>, DatabaseError>;
    async fn get_class(&self, name: &str) -> Result<Option<Class>, DatabaseError>;
    async fn create_class(&self, class: &Class) -> Result<(), DatabaseError>;

    async fn drop_database(&self) -> Result<(), DatabaseError>;
}

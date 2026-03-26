use crate::model::{Class, Object, Rule, Value};
use async_trait::async_trait;
use chrono::{DateTime, Utc};
use std::{collections::HashMap, sync::Arc};

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

#[async_trait]
pub trait Database: Send + Sync {
    fn name(&self) -> &str;

    async fn get_classes(&self) -> Result<Vec<Class>, DatabaseError>;
    async fn get_class(&self, name: &str) -> Result<Option<Class>, DatabaseError>;
    async fn create_class(&self, class: &Class) -> Result<(), DatabaseError>;

    async fn get_objects(&self) -> Result<Vec<Object>, DatabaseError>;
    async fn create_object(&self, object: &Object) -> Result<String, DatabaseError>;
    async fn add_class(&self, object_id: &str, class_name: &str) -> Result<(), DatabaseError>;
    async fn set_properties(&self, object_id: &str, properties: &HashMap<String, Value>) -> Result<(), DatabaseError>;
    async fn add_data(&self, object_id: &str, values: &HashMap<String, Value>, date_time: &DateTime<Utc>) -> Result<(), DatabaseError>;
    async fn get_data(&self, object_id: &str, start_time: Option<&DateTime<Utc>>, end_time: Option<&DateTime<Utc>>) -> Result<Vec<(HashMap<String, Value>, DateTime<Utc>)>, DatabaseError>;

    async fn get_rules(&self) -> Result<Vec<Rule>, DatabaseError>;
    async fn create_rule(&self, rule: &Rule) -> Result<(), DatabaseError>;

    async fn add_fcm_token(&self, object_id: &str, token: &str) -> Result<(), DatabaseError>;
    async fn remove_fcm_token(&self, object_id: &str, token: &str) -> Result<(), DatabaseError>;
    async fn get_fcm_tokens(&self, object_id: &str) -> Result<Vec<String>, DatabaseError>;

    async fn drop_database(&self) -> Result<(), DatabaseError>;
}

pub async fn setup_db() -> Arc<dyn Database> {
    #[cfg(feature = "mongodb")]
    return setup_mongodb().await;

    #[cfg(not(feature = "mongodb"))]
    panic!("No database backend configured");
}

#[cfg(feature = "mongodb")]
async fn setup_mongodb() -> Arc<dyn Database> {
    use crate::db::mongodb::MongoDB;

    let name = std::env::var("DB_NAME").unwrap_or_else(|_| "coco_db".to_owned());
    let host = std::env::var("DB_HOST").unwrap_or_else(|_| "localhost".to_owned());
    let port = std::env::var("DB_PORT").ok().and_then(|p| p.parse().ok()).unwrap_or(27017);
    let uri = format!("mongodb://{}:{}", host, port);
    Arc::new(MongoDB::new(&name, &uri).await.unwrap())
}

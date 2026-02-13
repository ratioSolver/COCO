use crate::model::{Class, Object, Rule, Value};
use async_trait::async_trait;
use chrono::{DateTime, Utc};
use std::collections::HashMap;

#[cfg(feature = "mongodb")]
pub mod mongodb;

#[derive(Debug)]
pub enum DatabaseError {
    ConnectionError(String),
    ClassNotFound(String),
    ClassAlreadyExists(String),
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
    async fn get_values(&self, object_id: &str, from: &DateTime<Utc>, to: &DateTime<Utc>) -> Result<HashMap<String, Vec<(Value, DateTime<Utc>)>>, DatabaseError>;
    async fn add_data(&self, object_id: &str, values: &HashMap<String, Value>, date_time: &DateTime<Utc>) -> Result<(), DatabaseError>;

    async fn get_rules(&self) -> Result<Vec<Rule>, DatabaseError>;
    async fn create_rule(&self, rule: &Rule) -> Result<(), DatabaseError>;

    async fn drop_database(&self) -> Result<(), DatabaseError>;
}

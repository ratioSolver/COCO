use async_trait::async_trait;
use std::{collections::HashMap, error::Error};

pub struct DBPropertyType {
    pub name: String,
}

pub struct DBClass {
    pub name: String,
    pub static_properties: HashMap<String, DBPropertyType>,
}

pub struct DBObject {
    pub id: String,
    pub classes: Vec<String>,
}

pub struct DBRule {
    pub name: String,
    pub content: String,
}

#[async_trait]
pub trait Database {
    fn name(&self) -> &str;

    async fn get_classes(&self) -> Result<Vec<DBClass>, Box<dyn Error>>;
    async fn create_class(&self, kind: &DBClass) -> Result<(), Box<dyn Error>>;
    async fn get_objects(&self) -> Result<Vec<DBObject>, Box<dyn Error>>;
    async fn create_object(&self, object: &DBObject) -> Result<(), Box<dyn Error>>;
    async fn get_rules(&self) -> Result<Vec<DBRule>, Box<dyn Error>>;
    async fn create_rule(&self, rule: &DBRule) -> Result<(), Box<dyn Error>>;

    async fn drop_db(&self) -> Result<(), Box<dyn Error>>;
}

use async_trait::async_trait;
use std::error::Error;

pub struct DBKind {
    pub name: String,
}

pub struct DBItem {
    pub id: String,
    pub kinds: Vec<String>,
}

pub struct DBRule {
    pub name: String,
    pub content: String,
}

#[async_trait]
pub trait Database {
    fn name(&self) -> &str;

    async fn get_types(&self) -> Result<Vec<DBKind>, Box<dyn Error>>;
    async fn create_type(&self, kind: &DBKind) -> Result<(), Box<dyn Error>>;
    async fn get_items(&self) -> Result<Vec<DBItem>, Box<dyn Error>>;
    async fn create_item(&self, item: &DBItem) -> Result<(), Box<dyn Error>>;
    async fn get_rules(&self) -> Result<Vec<DBRule>, Box<dyn Error>>;
    async fn create_rule(&self, rule: &DBRule) -> Result<(), Box<dyn Error>>;
}

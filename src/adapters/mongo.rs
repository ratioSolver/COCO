use crate::DataStore;
use async_trait::async_trait;

pub struct MongoDBDataStore {}

impl MongoDBDataStore {
    pub fn new() -> Self {
        MongoDBDataStore {}
    }
}

#[async_trait]
impl DataStore for MongoDBDataStore {
    async fn add_class(&self, class_name: &str) -> Result<(), String> {
        // Implement the logic to add a class to MongoDB
        Ok(())
    }
}

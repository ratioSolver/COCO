use async_trait::async_trait;

#[async_trait]
pub trait DataStore: Send + Sync {
    async fn add_class(&self, class_name: &str) -> Result<(), String>;
}

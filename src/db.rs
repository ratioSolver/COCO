use async_trait::async_trait;

#[async_trait]
pub trait DataStore: Send + Sync {}

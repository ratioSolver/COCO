use async_trait::async_trait;

// Il Database deve essere asincrono e clonabile
#[async_trait]
pub trait Database: Clone + Send + Sync + 'static {}

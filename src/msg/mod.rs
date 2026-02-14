use async_trait::async_trait;

#[cfg(feature = "fcm")]
pub mod fcm;

#[derive(Debug)]
pub enum MessagingError {
    ConnectionError(String),
}

#[async_trait]
pub trait Messaging: Send + Sync {
    async fn send_message(&self, tokens: Vec<String>, title: &str, message: &str) -> Result<Vec<String>, MessagingError>;
}

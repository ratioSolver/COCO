use async_trait::async_trait;
#[cfg(feature = "fcm")]
mod fcm;

#[derive(Debug)]
pub enum MessagingError {
    ConnectionError(String),
}

#[async_trait]
pub trait Messaging: Send + Sync {
    async fn send_message(&self, tokens: Vec<String>, title: &str, message: &str) -> Result<Vec<String>, MessagingError>;
}

pub fn setup_messaging() -> Option<Box<dyn Messaging>> {
    #[cfg(feature = "fcm")]
    return Some(setup_fcm());

    #[cfg(not(feature = "fcm"))]
    None
}

#[cfg(feature = "fcm")]
fn setup_fcm() -> Box<dyn Messaging> {
    use crate::msg::fcm::FCMClient;

    let project_id = std::env::var("FCM_PROJECT_ID").unwrap_or_else(|_| "coco-project-id".to_owned());
    Box::new(FCMClient::new(project_id))
}

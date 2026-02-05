use async_trait::async_trait;
use tokio::sync::broadcast;

use crate::coco::CoCoEvent;

#[async_trait]
pub trait KnowledgeBase: Send + Sync {
    fn register_callback(&mut self, sender: broadcast::Sender<CoCoEvent>);
    fn add_class(&mut self, class_name: &str);
}

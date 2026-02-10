use crate::model::Class;

#[derive(Debug)]
pub enum KnowledgeBaseError {
    ClassAlreadyExists(String),
    ClassNotFound(String),
}

pub trait KnowledgeBase {
    fn get_classes(&self) -> Vec<&Class>;
    fn get_class(&self, name: &str) -> Option<&Class>;
    fn create_class(&mut self, class: &Class) -> Result<(), KnowledgeBaseError>;
}

#[cfg(feature = "clips")]
pub mod clips;

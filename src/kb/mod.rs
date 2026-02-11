use crate::model::Class;

#[cfg(feature = "clips")]
pub mod clips;

#[derive(Debug)]
pub enum KnowledgeBaseError {
    ClassAlreadyExists(String),
    ClassNotFound(String),
    KBError(String),
}

pub trait KnowledgeBase {
    fn get_classes(&self) -> Vec<&Class>;
    fn get_class(&self, name: &str) -> Option<&Class>;
    fn create_class(&mut self, class: &Class) -> Result<(), KnowledgeBaseError>;
}

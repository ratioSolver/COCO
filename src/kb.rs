use crate::db::Class;
use std::error::Error;

pub trait KnowledgeBase {
    fn create_class(&self, class: &Class) -> Result<(), Box<dyn Error>>;
}

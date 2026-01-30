use crate::db::{Class, Object};
use std::error::Error;

pub trait KnowledgeBase {
    fn create_class(&self, class: &Class) -> Result<(), Box<dyn Error>>;
    fn create_object(&self, class: &Class, object: &Object) -> Result<(), Box<dyn Error>>;
}

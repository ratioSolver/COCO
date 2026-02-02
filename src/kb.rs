use chrono::{DateTime, Utc};

use crate::{
    Value,
    db::{Class, Object},
};
use std::error::Error;

pub trait KnowledgeBase {
    fn create_class(&self, class: &Class) -> Result<(), Box<dyn Error>>;
    fn create_object(&mut self, class: &Class, object: &Object) -> Result<(), Box<dyn Error>>;
    fn add_data(&mut self, class: &Class, object: &Object, values: Vec<(&str, &Value)>, date_time: DateTime<Utc>) -> Result<(), Box<dyn Error>>;
}

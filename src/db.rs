use async_trait::async_trait;
use serde::{Deserialize, Serialize};
use std::{collections::{HashMap, HashSet}, error::Error};

use crate::Property;

#[derive(Serialize, Deserialize, Debug)]
pub struct Class {
    pub name: String,
    pub static_properties: HashMap<String, Property>,
    pub dynamic_properties: HashMap<String, Property>,
}

#[derive(Serialize, Deserialize, Debug)]
pub struct Object {
    pub id: String,
    pub classes: HashSet<String>,
}

#[derive(Serialize, Deserialize, Debug)]
pub struct Rule {
    pub name: String,
    pub content: String,
}

#[async_trait]
pub trait Database {
    fn name(&self) -> &str;

    async fn get_classes(&self) -> Result<Vec<Class>, Box<dyn Error>>;
    async fn create_class(&self, kind: &Class) -> Result<(), Box<dyn Error>>;
    async fn get_objects(&self) -> Result<Vec<Object>, Box<dyn Error>>;
    async fn create_object(&self, object: &Object) -> Result<(), Box<dyn Error>>;
    async fn get_rules(&self) -> Result<Vec<Rule>, Box<dyn Error>>;
    async fn create_rule(&self, rule: &Rule) -> Result<(), Box<dyn Error>>;

    async fn drop_db(&self) -> Result<(), Box<dyn Error>>;
}

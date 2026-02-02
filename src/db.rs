use async_trait::async_trait;
use chrono::{DateTime, Utc};
use serde::{Deserialize, Serialize};
use std::{
    collections::{HashMap, HashSet},
    error::Error,
};

#[derive(Clone, Serialize, Deserialize, Debug)]
#[serde(tag = "type")]
pub enum Property {
    #[serde(rename = "bool")]
    Bool {
        #[serde(skip_serializing_if = "Option::is_none")]
        nullable: Option<bool>,
        #[serde(skip_serializing_if = "Option::is_none")]
        default: Option<bool>,
    },
    #[serde(rename = "int")]
    Int {
        #[serde(skip_serializing_if = "Option::is_none")]
        nullable: Option<bool>,
        #[serde(skip_serializing_if = "Option::is_none")]
        default: Option<i64>,
        #[serde(skip_serializing_if = "Option::is_none")]
        min: Option<i64>,
        #[serde(skip_serializing_if = "Option::is_none")]
        max: Option<i64>,
    },
    #[serde(rename = "float")]
    Float {
        #[serde(skip_serializing_if = "Option::is_none")]
        nullable: Option<bool>,
        #[serde(skip_serializing_if = "Option::is_none")]
        default: Option<f64>,
        #[serde(skip_serializing_if = "Option::is_none")]
        min: Option<f64>,
        #[serde(skip_serializing_if = "Option::is_none")]
        max: Option<f64>,
    },
}

#[derive(Clone, Serialize, Deserialize, Debug)]
#[serde(untagged)]
pub enum Value {
    Bool(bool),
    Int(i64),
    Float(f64),
}

#[derive(Clone, Serialize, Deserialize, Debug)]
pub struct Class {
    pub name: String,
    pub parents: Option<HashSet<String>>,
    pub static_properties: Option<HashMap<String, Property>>,
    pub dynamic_properties: Option<HashMap<String, Property>>,
}

#[derive(Clone, Serialize, Deserialize, Debug)]
pub struct Object {
    pub id: String,
    pub classes: Option<HashSet<String>>,
    pub properties: Option<HashMap<String, Value>>,
    pub values: Option<HashMap<String, (Value, DateTime<Utc>)>>,
}

#[derive(Clone, Serialize, Deserialize, Debug)]
pub struct Rule {
    pub name: String,
    pub content: String,
}

#[async_trait]
pub trait Database {
    fn name(&self) -> &str;

    async fn get_classes(&self) -> Result<Vec<Class>, Box<dyn Error>>;
    async fn create_class(&self, class: &Class) -> Result<(), Box<dyn Error>>;
    async fn get_objects(&self) -> Result<Vec<Object>, Box<dyn Error>>;
    async fn create_object(&self, object: &Object) -> Result<(), Box<dyn Error>>;
    async fn get_rules(&self) -> Result<Vec<Rule>, Box<dyn Error>>;
    async fn create_rule(&self, rule: &Rule) -> Result<(), Box<dyn Error>>;

    async fn drop_db(&self) -> Result<(), Box<dyn Error>>;
}

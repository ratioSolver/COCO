mod adapters;
mod coco;
pub use adapters::clips::CLIPSKnowledgeBase;
pub use adapters::mongo::MongoDBDataStore;
use async_trait::async_trait;
use chrono::{DateTime, Utc};
pub use coco::CoCo;
use serde::{Deserialize, Serialize};
use std::{
    collections::{HashMap, HashSet},
    error::Error,
    fmt::{Display, Formatter},
};
use tokio::sync::broadcast;
use utoipa::ToSchema;

#[derive(Clone, Serialize, Deserialize, Debug, PartialEq, ToSchema)]
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
    #[serde(rename = "string")]
    String {
        #[serde(skip_serializing_if = "Option::is_none")]
        nullable: Option<bool>,
        #[serde(skip_serializing_if = "Option::is_none")]
        default: Option<String>,
    },
    #[serde(rename = "symbol")]
    Symbol {
        #[serde(skip_serializing_if = "Option::is_none")]
        nullable: Option<bool>,
        #[serde(skip_serializing_if = "Option::is_none")]
        default: Option<String>,
        #[serde(skip_serializing_if = "Option::is_none")]
        allowed_values: Option<HashSet<String>>,
    },
    #[serde(rename = "object")]
    Object {
        #[serde(skip_serializing_if = "Option::is_none")]
        nullable: Option<bool>,
        #[serde(skip_serializing_if = "Option::is_none")]
        default: Option<String>,
        class: String,
    },
    BoolArray {
        #[serde(skip_serializing_if = "Option::is_none")]
        default: Option<Vec<bool>>,
    },
    IntArray {
        #[serde(skip_serializing_if = "Option::is_none")]
        default: Option<Vec<i64>>,
        #[serde(skip_serializing_if = "Option::is_none")]
        min: Option<i64>,
        #[serde(skip_serializing_if = "Option::is_none")]
        max: Option<i64>,
    },
    FloatArray {
        #[serde(skip_serializing_if = "Option::is_none")]
        default: Option<Vec<f64>>,
        #[serde(skip_serializing_if = "Option::is_none")]
        min: Option<f64>,
        #[serde(skip_serializing_if = "Option::is_none")]
        max: Option<f64>,
    },
    StringArray {
        #[serde(skip_serializing_if = "Option::is_none")]
        default: Option<Vec<String>>,
    },
    SymbolArray {
        #[serde(skip_serializing_if = "Option::is_none")]
        default: Option<Vec<String>>,
        #[serde(skip_serializing_if = "Option::is_none")]
        allowed_values: Option<HashSet<String>>,
    },
    ObjectArray {
        #[serde(skip_serializing_if = "Option::is_none")]
        default: Option<Vec<String>>,
        class: String,
    },
}

impl Display for Property {
    fn fmt(&self, f: &mut Formatter<'_>) -> std::fmt::Result {
        match self {
            Property::Bool { nullable, default } => {
                write!(f, "bool(nullable: {:?}, default: {:?})", nullable, default)
            }
            Property::Int { nullable, default, min, max } => {
                write!(f, "int(nullable: {:?}, default: {:?}, min: {:?}, max: {:?})", nullable, default, min, max)
            }
            Property::Float { nullable, default, min, max } => {
                write!(f, "float(nullable: {:?}, default: {:?}, min: {:?}, max: {:?})", nullable, default, min, max)
            }
            Property::String { nullable, default } => {
                write!(f, "string(nullable: {:?}, default: {:?})", nullable, default)
            }
            Property::Symbol { nullable, default, allowed_values } => {
                write!(f, "symbol(nullable: {:?}, default: {:?}, allowed_values: {:?})", nullable, default, allowed_values)
            }
            Property::Object { nullable, default, class } => {
                write!(f, "object(nullable: {:?}, default: {:?}, class: {:?})", nullable, default, class)
            }
            Property::BoolArray { default } => {
                write!(f, "bool_array(default: {:?})", default)
            }
            Property::IntArray { default, min, max } => {
                write!(f, "int_array(default: {:?}, min: {:?}, max: {:?})", default, min, max)
            }
            Property::FloatArray { default, min, max } => {
                write!(f, "float_array(default: {:?}, min: {:?}, max: {:?})", default, min, max)
            }
            Property::StringArray { default } => {
                write!(f, "string_array(default: {:?})", default)
            }
            Property::SymbolArray { default, allowed_values } => {
                write!(f, "symbol_array(default: {:?}, allowed_values: {:?})", default, allowed_values)
            }
            Property::ObjectArray { default, class } => {
                write!(f, "object_array(default: {:?}, class: {:?})", default, class)
            }
        }
    }
}

#[derive(Clone, Serialize, Deserialize, Debug, PartialEq)]
#[serde(untagged)]
pub enum Value {
    Null,
    Bool(bool),
    Int(i64),
    Float(f64),
    String(String),
    Symbol(String),
    Object(String),
    BoolArray(Vec<bool>),
    IntArray(Vec<i64>),
    FloatArray(Vec<f64>),
    StringArray(Vec<String>),
}

impl Display for Value {
    fn fmt(&self, f: &mut Formatter<'_>) -> std::fmt::Result {
        match self {
            Value::Null => write!(f, "null"),
            Value::Bool(b) => write!(f, "{}", b),
            Value::Int(i) => write!(f, "{}", i),
            Value::Float(fl) => write!(f, "{}", fl),
            Value::String(s) => write!(f, "\"{}\"", s),
            Value::Symbol(s) => write!(f, "'{}'", s),
            Value::Object(o) => write!(f, "object_id: {}", o),
            Value::BoolArray(arr) => write!(f, "bool_array: {:?}", arr),
            Value::IntArray(arr) => write!(f, "int_array: {:?}", arr),
            Value::FloatArray(arr) => write!(f, "float_array: {:?}", arr),
            Value::StringArray(arr) => write!(f, "string_array: {:?}", arr),
        }
    }
}

#[derive(Clone, Serialize, Deserialize, Debug, ToSchema)]
pub struct Class {
    pub name: String,
    pub parents: Option<HashSet<String>>,
    pub static_properties: Option<HashMap<String, Property>>,
    pub dynamic_properties: Option<HashMap<String, Property>>,
}

impl Display for Class {
    fn fmt(&self, f: &mut Formatter<'_>) -> std::fmt::Result {
        write!(f, "class {} parents: {:?} static_properties: {:?} dynamic_properties: {:?}", self.name, self.parents, self.static_properties, self.dynamic_properties)
    }
}

#[derive(Clone, Serialize, Deserialize, Debug, ToSchema)]
pub struct Object {
    pub id: Option<String>,
    pub classes: HashSet<String>,
    #[serde(skip_serializing_if = "Option::is_none")]
    pub properties: Option<HashMap<String, Value>>,
    #[serde(skip_serializing_if = "Option::is_none")]
    pub values: Option<HashMap<String, (Value, DateTime<Utc>)>>,
}

impl Display for Object {
    fn fmt(&self, f: &mut Formatter<'_>) -> std::fmt::Result {
        write!(f, "object {} classes: {:?} properties: {:?} values: {:?}", self.id.as_deref().unwrap_or(""), self.classes, self.properties, self.values)
    }
}

#[derive(Clone, Serialize, Deserialize, Debug, ToSchema)]
pub struct Rule {
    pub name: String,
    pub content: String,
}

impl Display for Rule {
    fn fmt(&self, f: &mut Formatter<'_>) -> std::fmt::Result {
        write!(f, "rule {} content: {}", self.name, self.content)
    }
}

#[derive(Clone, Debug)]
pub enum CoCoEvent {
    ClassCreated(Class),
    ObjectCreated(Object),
    AddedClass(String, String),                                   // (object_id, class_name)
    AddedValues(String, HashMap<String, (Value, DateTime<Utc>)>), // (object_id, values)
}

impl Display for CoCoEvent {
    fn fmt(&self, f: &mut Formatter<'_>) -> std::fmt::Result {
        match self {
            CoCoEvent::ClassCreated(class) => write!(f, "ClassCreated: {}", class),
            CoCoEvent::ObjectCreated(object) => write!(f, "ObjectCreated: {}", object),
            CoCoEvent::AddedClass(object_id, class_name) => write!(f, "AddedClass: {} to {}", class_name, object_id),
            CoCoEvent::AddedValues(object_id, values) => write!(f, "AddedValues to {}: {:?}", object_id, values),
        }
    }
}

pub trait KnowledgeBase: Send + Sync {
    fn get_event_sender(&self) -> broadcast::Sender<CoCoEvent>;
    fn create_class(&self, class: &Class) -> Result<(), Box<dyn Error>>;
    fn create_object(&self, class: &Class, object: &Object) -> Result<(), Box<dyn Error>>;
    fn set_properties(&self, class: &Class, object: &Object, values: &HashMap<String, Value>) -> Result<(), Box<dyn Error>>;
    fn add_data(&self, class: &Class, object: &Object, values: &HashMap<String, Value>, date_time: &DateTime<Utc>) -> Result<(), Box<dyn Error>>;
    fn create_rule(&self, rule: &Rule) -> Result<(), Box<dyn Error>>;
}

#[async_trait]
pub trait DataStore: Send + Sync {
    fn name(&self) -> &str;

    async fn get_classes(&self) -> Result<Vec<Class>, Box<dyn Error>>;
    async fn create_class(&self, class: &Class) -> Result<(), Box<dyn Error>>;

    async fn get_objects(&self) -> Result<Vec<Object>, Box<dyn Error>>;
    async fn create_object(&self, object: &Object) -> Result<String, Box<dyn Error>>;
    async fn set_properties(&self, object: &Object, properties: &HashMap<String, Value>) -> Result<(), Box<dyn Error>>;
    async fn get_values(&self, object: &Object, from: &DateTime<Utc>, to: &DateTime<Utc>) -> Result<HashMap<String, Vec<(Value, DateTime<Utc>)>>, Box<dyn Error>>;
    async fn set_values(&self, object: &Object, values: &HashMap<String, Value>, date_time: &DateTime<Utc>) -> Result<(), Box<dyn Error>>;

    async fn get_rules(&self) -> Result<Vec<Rule>, Box<dyn Error>>;
    async fn create_rule(&self, rule: &Rule) -> Result<(), Box<dyn Error>>;

    async fn drop_db(&self) -> Result<(), Box<dyn Error>>;
}

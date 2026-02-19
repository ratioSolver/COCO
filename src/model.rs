use chrono::{DateTime, Utc};
use serde::{Deserialize, Serialize};
use std::{
    collections::{HashMap, HashSet},
    fmt::{Display, Formatter},
};
#[cfg(feature = "server")]
use utoipa::ToSchema;

#[derive(Clone, Serialize, Deserialize, Debug, PartialEq)]
#[cfg_attr(feature = "server", derive(ToSchema))]
#[serde(tag = "type")]
pub enum Property {
    #[serde(rename = "bool")]
    Bool {
        #[serde(skip_serializing_if = "Option::is_none")]
        default: Option<bool>,
    },
    #[serde(rename = "int")]
    Int {
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
        default: Option<f64>,
        #[serde(skip_serializing_if = "Option::is_none")]
        min: Option<f64>,
        #[serde(skip_serializing_if = "Option::is_none")]
        max: Option<f64>,
    },
    #[serde(rename = "string")]
    String {
        #[serde(skip_serializing_if = "Option::is_none")]
        default: Option<String>,
    },
    #[serde(rename = "symbol")]
    Symbol {
        #[serde(skip_serializing_if = "Option::is_none")]
        default: Option<String>,
        #[serde(skip_serializing_if = "Option::is_none")]
        allowed_values: Option<HashSet<String>>,
    },
    #[serde(rename = "object")]
    Object {
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
            Property::Bool { default } => {
                write!(f, "bool(default: {:?})", default)
            }
            Property::Int { default, min, max } => {
                write!(f, "int(default: {:?}, min: {:?}, max: {:?})", default, min, max)
            }
            Property::Float { default, min, max } => {
                write!(f, "float(default: {:?}, min: {:?}, max: {:?})", default, min, max)
            }
            Property::String { default } => {
                write!(f, "string(default: {:?})", default)
            }
            Property::Symbol { default, allowed_values } => {
                write!(f, "symbol(default: {:?}, allowed_values: {:?})", default, allowed_values)
            }
            Property::Object { default, class } => {
                write!(f, "object(default: {:?}, class: {:?})", default, class)
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
#[cfg_attr(feature = "server", derive(ToSchema))]
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

#[derive(Clone, Serialize, Deserialize, Debug, PartialEq)]
#[cfg_attr(feature = "server", derive(ToSchema))]
pub struct TimedValue {
    pub value: Value,
    pub timestamp: DateTime<Utc>,
}

impl PartialEq<&str> for Value {
    fn eq(&self, other: &&str) -> bool {
        match self {
            Value::Null => *other == "null",
            Value::Bool(b) => other == &b.to_string(),
            Value::Int(i) => other == &i.to_string(),
            Value::Float(f) => other == &f.to_string(),
            Value::String(s) => other == s,
            Value::Symbol(s) => other == s,
            Value::Object(o) => other == o,
            Value::BoolArray(arr) => other == &format!("{:?}", arr),
            Value::IntArray(arr) => other == &format!("{:?}", arr),
            Value::FloatArray(arr) => other == &format!("{:?}", arr),
            Value::StringArray(arr) => other == &format!("{:?}", arr),
        }
    }
}

impl PartialEq<String> for Value {
    fn eq(&self, other: &String) -> bool {
        match self {
            Value::Null => other == "null",
            Value::Bool(b) => other == &b.to_string(),
            Value::Int(i) => other == &i.to_string(),
            Value::Float(f) => other == &f.to_string(),
            Value::String(s) => other == s,
            Value::Symbol(s) => other == s,
            Value::Object(o) => other == o,
            Value::BoolArray(arr) => other == &format!("{:?}", arr),
            Value::IntArray(arr) => other == &format!("{:?}", arr),
            Value::FloatArray(arr) => other == &format!("{:?}", arr),
            Value::StringArray(arr) => other == &format!("{:?}", arr),
        }
    }
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

#[derive(Clone, Serialize, Deserialize, Debug)]
#[cfg_attr(feature = "server", derive(ToSchema))]
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

#[derive(Clone, Serialize, Deserialize, Debug)]
#[cfg_attr(feature = "server", derive(ToSchema))]
pub struct Object {
    pub id: Option<String>,
    pub classes: HashSet<String>,
    #[serde(skip_serializing_if = "Option::is_none")]
    pub properties: Option<HashMap<String, Value>>,
    #[serde(skip_serializing_if = "Option::is_none")]
    pub values: Option<HashMap<String, TimedValue>>,
}

impl Display for Object {
    fn fmt(&self, f: &mut Formatter<'_>) -> std::fmt::Result {
        write!(f, "object {} classes: {:?} properties: {:?} values: {:?}", self.id.as_deref().unwrap_or(""), self.classes, self.properties, self.values)
    }
}

#[derive(Clone, Serialize, Deserialize, Debug)]
#[cfg_attr(feature = "server", derive(ToSchema))]
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
    ClassCreated(String),                                         // class_name
    ObjectCreated(String),                                        // object_id
    AddedClass(String, String),                                   // (object_id, class_name)
    PendingClass(String, String),                                 // (object_id, class_name)
    UpdatedProperties(String, HashMap<String, Value>),            // (object_id, properties)
    AddedValues(String, HashMap<String, Value>, DateTime<Utc>),   // (object_id, value, date_time)
    PendingValues(String, HashMap<String, Value>, DateTime<Utc>), // (object_id, value, date_time)
    RuleCreated(String),                                          // rule_name
    LLMPrompt(String, String),                                    // (object_id, prompt)
    LLMResponse(String, String),                                  // (object_id, response)
    Message(String, String, String),                              // (object_id, title, message)
}

impl Display for CoCoEvent {
    fn fmt(&self, f: &mut Formatter<'_>) -> std::fmt::Result {
        match self {
            CoCoEvent::ClassCreated(class) => write!(f, "ClassCreated: {}", class),
            CoCoEvent::ObjectCreated(object) => write!(f, "ObjectCreated: {}", object),
            CoCoEvent::AddedClass(object, class) => write!(f, "AddedClass: {} to {}", class, object),
            CoCoEvent::PendingClass(object, class) => write!(f, "PendingClass: {} to {}", class, object),
            CoCoEvent::UpdatedProperties(object, properties) => write!(f, "UpdatedProperties for {}: {:?}", object, properties),
            CoCoEvent::AddedValues(object, values, date_time) => write!(f, "AddedValues to {}: {:?} at {}", object, values, date_time),
            CoCoEvent::PendingValues(object, values, date_time) => write!(f, "PendingValues to {}: {:?} at {}", object, values, date_time),
            CoCoEvent::RuleCreated(rule) => write!(f, "RuleCreated: {}", rule),
            CoCoEvent::LLMPrompt(object, prompt) => write!(f, "LLMPrompt to {}: {}", object, prompt),
            CoCoEvent::LLMResponse(object, response) => write!(f, "LLMResponse from {}: {}", object, response),
            CoCoEvent::Message(object, title, message) => write!(f, "FCMMessage to {}: {} - {}", object, title, message),
        }
    }
}

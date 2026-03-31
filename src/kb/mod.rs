use crate::model::{Class, Object, Rule, Value};
use chrono::{DateTime, Utc};
use std::{collections::HashMap, fmt};
use tokio::sync::oneshot;

pub enum Command {
    CreateClass(Class, oneshot::Sender<Result<(), KnowledgeBaseError>>),
    CreateObject(Object, oneshot::Sender<Result<(), KnowledgeBaseError>>),
    AddClass(String, String, oneshot::Sender<Result<(), KnowledgeBaseError>>),
    SetProperties(String, HashMap<String, Value>, oneshot::Sender<Result<(), KnowledgeBaseError>>),
    AddValues(String, HashMap<String, Value>, DateTime<Utc>, oneshot::Sender<Result<(), KnowledgeBaseError>>),
    CreateRule(Rule, oneshot::Sender<Result<(), KnowledgeBaseError>>),
    SetLLMResult(String, String, oneshot::Sender<Result<(), KnowledgeBaseError>>),
    Run(oneshot::Sender<Result<(), KnowledgeBaseError>>),
}

#[derive(Debug)]
pub enum KnowledgeBaseError {
    CreationError(String),
    ClassAlreadyExists(String),
    ClassNotFound(String),
    ObjectAlreadyExists(String),
    ObjectNotFound(String),
    RuleAlreadyExists(String),
    RuleNotFound(String),
    KBError(String),
}

impl fmt::Display for KnowledgeBaseError {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        match self {
            KnowledgeBaseError::CreationError(msg) => write!(f, "Creation error: {}", msg),
            KnowledgeBaseError::ClassAlreadyExists(name) => write!(f, "Class already exists: {}", name),
            KnowledgeBaseError::ClassNotFound(name) => write!(f, "Class not found: {}", name),
            KnowledgeBaseError::ObjectAlreadyExists(id) => write!(f, "Object already exists: {}", id),
            KnowledgeBaseError::ObjectNotFound(id) => write!(f, "Object not found: {}", id),
            KnowledgeBaseError::RuleAlreadyExists(name) => write!(f, "Rule already exists: {}", name),
            KnowledgeBaseError::RuleNotFound(name) => write!(f, "Rule not found: {}", name),
            KnowledgeBaseError::KBError(msg) => write!(f, "Knowledge base error: {}", msg),
        }
    }
}

pub trait KnowledgeBase: Clone + Send + Sync + 'static {}

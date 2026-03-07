use crate::{
    db::{Database, DatabaseError, setup_db},
    kb::{KnowledgeBase, KnowledgeBaseError, setup_kb},
    llm::{LLM, setup_llm},
    model::{Class, CoCoEvent, Object, Property, Rule, TimedValue, Value},
    msg::{Messaging, setup_messaging},
};
use chrono::{DateTime, Utc};
use std::{
    collections::{HashMap, HashSet},
    path::{Path, PathBuf},
    sync::Arc,
};
use tokio::{
    fs,
    sync::{broadcast, mpsc, oneshot},
};
pub use tracing;
use tracing::{info, trace};
pub use tracing_subscriber;

pub mod db;
pub mod kb;
pub mod llm;
pub mod model;
#[cfg(feature = "mqtt")]
pub mod mqtt;
pub mod msg;
#[cfg(feature = "server")]
pub mod server;

pub struct CoCo {
    kb_tx: mpsc::Sender<CoCoCommand>,
    event_tx: broadcast::Sender<CoCoEvent>,
}

pub trait CoCoState: Clone + Send + Sync + 'static {
    fn coco(&self) -> Arc<CoCo>;
}

#[derive(Clone, Debug)]
pub enum CoCoError {
    DirectoryReadError(String),
    FileReadError(String),
    JsonParseError(String),
    ClassAlreadyExists(String),
    ClassNotFound(String),
    ObjectAlreadyExists(String),
    ObjectNotFound(String),
    RuleAlreadyExists(String),
    RuleNotFound(String),
    DatabaseError(String),
    KnowledgeBaseError(String),
    LLMError(String),
    MessagingError(String),
}

impl std::fmt::Display for CoCoError {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        match self {
            CoCoError::DirectoryReadError(msg) => write!(f, "Failed to read directory: {}", msg),
            CoCoError::FileReadError(msg) => write!(f, "Failed to read file: {}", msg),
            CoCoError::JsonParseError(msg) => write!(f, "Failed to parse JSON: {}", msg),
            CoCoError::ClassAlreadyExists(msg) => write!(f, "Class already exists: {}", msg),
            CoCoError::ClassNotFound(msg) => write!(f, "Class not found: {}", msg),
            CoCoError::ObjectAlreadyExists(msg) => write!(f, "Object already exists: {}", msg),
            CoCoError::ObjectNotFound(msg) => write!(f, "Object not found: {}", msg),
            CoCoError::RuleAlreadyExists(msg) => write!(f, "Rule already exists: {}", msg),
            CoCoError::RuleNotFound(msg) => write!(f, "Rule not found: {}", msg),
            CoCoError::DatabaseError(msg) => write!(f, "Database error: {}", msg),
            CoCoError::KnowledgeBaseError(msg) => write!(f, "Knowledge base error: {}", msg),
            CoCoError::LLMError(msg) => write!(f, "LLM error: {}", msg),
            CoCoError::MessagingError(msg) => write!(f, "Messaging error: {}", msg),
        }
    }
}

enum CoCoCommand {
    InitData { classes: Vec<Class>, objects: Vec<Object>, rules: Vec<Rule>, resp: oneshot::Sender<()> },
    LoadClasses { path: PathBuf, resp: oneshot::Sender<Result<(), CoCoError>> },
    LoadClass { class_definition: String, resp: oneshot::Sender<Result<(), CoCoError>> },
    GetClasses { resp: oneshot::Sender<Vec<Class>> },
    GetClass { name: String, resp: oneshot::Sender<Option<Class>> },
    CreateClass { class: Class, resp: oneshot::Sender<Result<(), CoCoError>> },
    LoadObjects { path: PathBuf, resp: oneshot::Sender<Result<(), CoCoError>> },
    LoadObject { object_definition: String, resp: oneshot::Sender<Result<(), CoCoError>> },
    GetObjects { resp: oneshot::Sender<Vec<Object>> },
    GetObject { id: String, resp: oneshot::Sender<Option<Object>> },
    CreateObject { object: Object, resp: oneshot::Sender<Result<String, CoCoError>> },
    SetProperties { object_id: String, values: HashMap<String, Value>, resp: oneshot::Sender<Result<(), CoCoError>> },
    AddData { object_id: String, values: HashMap<String, Value>, date_time: DateTime<Utc>, resp: oneshot::Sender<Result<(), CoCoError>> },
    GetData { object_id: String, start_time: Option<DateTime<Utc>>, end_time: Option<DateTime<Utc>>, resp: oneshot::Sender<Result<Vec<(HashMap<String, Value>, DateTime<Utc>)>, CoCoError>> },
    LoadRules { path: PathBuf, resp: oneshot::Sender<Result<(), CoCoError>> },
    LoadRule { rule_definition: String, resp: oneshot::Sender<Result<(), CoCoError>> },
    GetRules { resp: oneshot::Sender<Vec<Rule>> },
    GetRule { name: String, resp: oneshot::Sender<Option<Rule>> },
    CreateRule { rule: Rule, resp: oneshot::Sender<Result<(), CoCoError>> },
}

impl CoCo {
    pub async fn new(db: Arc<dyn Database>, mut kb: Box<dyn KnowledgeBase>, llm: Option<Box<dyn LLM>>, fcm: Option<Box<dyn Messaging>>) -> Self {
        let (kb_tx, mut kb_rx) = mpsc::channel(64);
        let (event_tx, _event_rx) = broadcast::channel(64);
        let mut kb_event_rx = kb.get_event_sender().subscribe();
        let db_for_task = db.clone();
        let event_tx_task = event_tx.clone();

        tokio::spawn(async move {
            loop {
                tokio::select! {
                    Some(cmd) = kb_rx.recv() => {
                        handle_command(cmd, &db_for_task, &mut kb).await;
                    }
                    Ok(event) = kb_event_rx.recv() => {
                        handle_kb_event(event, &db_for_task, &mut kb, &llm, &fcm, &event_tx_task).await;
                    }
                }
            }
        });

        let classes = db.get_classes().await.unwrap_or_else(|e| {
            eprintln!("Error fetching classes from database: {:?}", e);
            vec![]
        });
        let objects = db.get_objects().await.unwrap_or_else(|e| {
            eprintln!("Error fetching objects from database: {:?}", e);
            vec![]
        });
        let rules = db.get_rules().await.unwrap_or_else(|e| {
            eprintln!("Error fetching rules from database: {:?}", e);
            vec![]
        });
        let (resp_tx, resp_rx) = oneshot::channel();
        let _ = kb_tx.send(CoCoCommand::InitData { classes, objects, rules, resp: resp_tx }).await;
        let _ = resp_rx.await;

        CoCo { kb_tx, event_tx }
    }

    pub async fn default() -> Self {
        CoCo::new(setup_db().await, setup_kb(), setup_llm(), setup_messaging()).await
    }

    pub fn get_event_sender(&self) -> broadcast::Sender<CoCoEvent> {
        self.event_tx.clone()
    }

    pub async fn load_classes<P: AsRef<Path>>(&self, path: P) -> Result<(), CoCoError> {
        let (resp_tx, resp_rx) = oneshot::channel();
        let _ = self.kb_tx.send(CoCoCommand::LoadClasses { path: path.as_ref().to_owned(), resp: resp_tx }).await;
        resp_rx.await.unwrap_or_else(|_| Err(CoCoError::DatabaseError("KB task closed".to_owned())))
    }

    pub async fn load_class(&self, class_definition: &str) -> Result<(), CoCoError> {
        let (resp_tx, resp_rx) = oneshot::channel();
        let _ = self.kb_tx.send(CoCoCommand::LoadClass { class_definition: class_definition.to_owned(), resp: resp_tx }).await;
        resp_rx.await.unwrap_or_else(|_| Err(CoCoError::DatabaseError("KB task closed".to_owned())))
    }

    pub async fn get_classes(&self) -> Vec<Class> {
        let (resp_tx, resp_rx) = oneshot::channel();
        let _ = self.kb_tx.send(CoCoCommand::GetClasses { resp: resp_tx }).await;
        resp_rx.await.unwrap_or_default()
    }

    pub async fn get_class(&self, name: &str) -> Option<Class> {
        let (resp_tx, resp_rx) = oneshot::channel();
        let _ = self.kb_tx.send(CoCoCommand::GetClass { name: name.to_owned(), resp: resp_tx }).await;
        resp_rx.await.unwrap_or(None)
    }

    pub async fn create_new_class(&self, name: &str, parents: Option<HashSet<String>>, static_properties: Option<HashMap<String, Property>>, dynamic_properties: Option<HashMap<String, Property>>) -> Result<(), CoCoError> {
        let class = Class { name: name.to_owned(), parents, static_properties, dynamic_properties };
        self.create_class(class).await
    }

    pub async fn create_class(&self, class: Class) -> Result<(), CoCoError> {
        let (resp_tx, resp_rx) = oneshot::channel();
        let _ = self.kb_tx.send(CoCoCommand::CreateClass { class, resp: resp_tx }).await;
        resp_rx.await.unwrap_or_else(|_| Err(CoCoError::DatabaseError("KB task closed".to_owned())))
    }

    pub async fn load_objects<P: AsRef<Path>>(&self, path: P) -> Result<(), CoCoError> {
        let (resp_tx, resp_rx) = oneshot::channel();
        let _ = self.kb_tx.send(CoCoCommand::LoadObjects { path: path.as_ref().to_owned(), resp: resp_tx }).await;
        resp_rx.await.unwrap_or_else(|_| Err(CoCoError::DatabaseError("KB task closed".to_owned())))
    }

    pub async fn load_object(&self, object_definition: &str) -> Result<(), CoCoError> {
        let (resp_tx, resp_rx) = oneshot::channel();
        let _ = self.kb_tx.send(CoCoCommand::LoadObject { object_definition: object_definition.to_owned(), resp: resp_tx }).await;
        resp_rx.await.unwrap_or_else(|_| Err(CoCoError::DatabaseError("KB task closed".to_owned())))
    }

    pub async fn get_objects(&self) -> Vec<Object> {
        let (resp_tx, resp_rx) = oneshot::channel();
        let _ = self.kb_tx.send(CoCoCommand::GetObjects { resp: resp_tx }).await;
        resp_rx.await.unwrap_or_default()
    }

    pub async fn get_object(&self, id: &str) -> Option<Object> {
        let (resp_tx, resp_rx) = oneshot::channel();
        let _ = self.kb_tx.send(CoCoCommand::GetObject { id: id.to_owned(), resp: resp_tx }).await;
        resp_rx.await.unwrap_or(None)
    }

    pub async fn create_new_object(&self, classes: HashSet<String>, properties: Option<HashMap<String, Value>>, values: Option<HashMap<String, TimedValue>>) -> Result<String, CoCoError> {
        self.create_object(Object { id: None, classes, properties, values }).await
    }

    pub async fn create_object(&self, object: Object) -> Result<String, CoCoError> {
        let (resp_tx, resp_rx) = oneshot::channel();
        let _ = self.kb_tx.send(CoCoCommand::CreateObject { object, resp: resp_tx }).await;
        resp_rx.await.unwrap_or_else(|_| Err(CoCoError::DatabaseError("KB task closed".to_owned())))
    }

    pub async fn set_properties(&self, object_id: &str, values: HashMap<String, Value>) -> Result<(), CoCoError> {
        let (resp_tx, resp_rx) = oneshot::channel();
        let _ = self.kb_tx.send(CoCoCommand::SetProperties { object_id: object_id.to_owned(), values, resp: resp_tx }).await;
        resp_rx.await.unwrap_or_else(|_| Err(CoCoError::DatabaseError("KB task closed".to_owned())))
    }

    pub async fn add_data(&self, object_id: &str, values: HashMap<String, Value>, date_time: DateTime<Utc>) -> Result<(), CoCoError> {
        let (resp_tx, resp_rx) = oneshot::channel();
        let _ = self.kb_tx.send(CoCoCommand::AddData { object_id: object_id.to_owned(), values, date_time, resp: resp_tx }).await;
        resp_rx.await.unwrap_or_else(|_| Err(CoCoError::DatabaseError("KB task closed".to_owned())))
    }

    pub async fn get_data(&self, object_id: &str, start_time: Option<DateTime<Utc>>, end_time: Option<DateTime<Utc>>) -> Result<Vec<(HashMap<String, Value>, DateTime<Utc>)>, CoCoError> {
        let (resp_tx, resp_rx) = oneshot::channel();
        let _ = self.kb_tx.send(CoCoCommand::GetData { object_id: object_id.to_owned(), start_time, end_time, resp: resp_tx }).await;
        resp_rx.await.unwrap_or_else(|_| Err(CoCoError::DatabaseError("KB task closed".to_owned())))
    }

    pub async fn load_rules<P: AsRef<Path>>(&self, path: P) -> Result<(), CoCoError> {
        let (resp_tx, resp_rx) = oneshot::channel();
        let _ = self.kb_tx.send(CoCoCommand::LoadRules { path: path.as_ref().to_owned(), resp: resp_tx }).await;
        resp_rx.await.unwrap_or_else(|_| Err(CoCoError::DatabaseError("KB task closed".to_owned())))
    }

    pub async fn load_rule(&self, rule_definition: &str) -> Result<(), CoCoError> {
        let (resp_tx, resp_rx) = oneshot::channel();
        let _ = self.kb_tx.send(CoCoCommand::LoadRule { rule_definition: rule_definition.to_owned(), resp: resp_tx }).await;
        resp_rx.await.unwrap_or_else(|_| Err(CoCoError::DatabaseError("KB task closed".to_owned())))
    }

    pub async fn get_rules(&self) -> Vec<Rule> {
        let (resp_tx, resp_rx) = oneshot::channel();
        let _ = self.kb_tx.send(CoCoCommand::GetRules { resp: resp_tx }).await;
        resp_rx.await.unwrap_or_default()
    }

    pub async fn get_rule(&self, name: &str) -> Option<Rule> {
        let (resp_tx, resp_rx) = oneshot::channel();
        let _ = self.kb_tx.send(CoCoCommand::GetRule { name: name.to_owned(), resp: resp_tx }).await;
        resp_rx.await.unwrap_or(None)
    }

    pub async fn create_new_rule(&self, name: &str, content: &str) -> Result<(), CoCoError> {
        self.create_rule(Rule { name: name.to_owned(), content: content.to_owned() }).await
    }

    pub async fn create_rule(&self, rule: Rule) -> Result<(), CoCoError> {
        let (resp_tx, resp_rx) = oneshot::channel();
        let _ = self.kb_tx.send(CoCoCommand::CreateRule { rule, resp: resp_tx }).await;
        resp_rx.await.unwrap_or_else(|_| Err(CoCoError::DatabaseError("KB task closed".to_owned())))
    }
}

async fn handle_command(cmd: CoCoCommand, db: &Arc<dyn Database>, kb: &mut Box<dyn KnowledgeBase>) {
    match cmd {
        CoCoCommand::InitData { classes, objects, rules, resp } => {
            info!("Initializing knowledge base with {} classes, {} objects and {} rules", classes.len(), objects.len(), rules.len());
            for class in classes {
                if let Err(e) = kb.create_class(class) {
                    eprintln!("Error adding class to knowledge base: {:?}", e);
                }
            }
            for object in objects {
                if let Err(e) = kb.create_object(object) {
                    eprintln!("Error adding object to knowledge base: {:?}", e);
                }
            }
            for rule in rules {
                if let Err(e) = kb.create_rule(rule) {
                    eprintln!("Error adding rule to knowledge base: {:?}", e);
                }
            }
            if let Err(e) = kb.run() {
                eprintln!("Error running knowledge base after initialization: {:?}", e);
            }
            let _ = resp.send(());
        }
        CoCoCommand::LoadClasses { path, resp } => {
            info!("Loading classes from directory '{}'", path.display());
            let result = async move {
                let mut entries = fs::read_dir(&path).await.map_err(|e| CoCoError::DirectoryReadError(format!("Failed to read directory '{}': {:?}", path.display(), e)))?;
                while let Some(entry) = entries.next_entry().await.map_err(|e| CoCoError::FileReadError(format!("Failed to read entry in directory '{}': {:?}", path.display(), e)))? {
                    let path = entry.path();
                    if path.extension().and_then(|s| s.to_str()) == Some("json") {
                        let data = fs::read_to_string(&path).await.map_err(|e| CoCoError::FileReadError(format!("Failed to read file '{}': {:?}", path.display(), e)))?;
                        let class: Class = serde_json::from_str(&data).map_err(|e| CoCoError::JsonParseError(format!("Failed to parse JSON in file '{}': {:?}", path.display(), e)))?;
                        if let None = kb.get_class(&class.name) {
                            info!("Creating class '{}' with parents {:?}, static properties {:?}, and dynamic properties {:?}", class.name, class.parents, class.static_properties, class.dynamic_properties);
                            db.create_class(&class).await.map_err(map_db_error)?;
                            kb.create_class(class).map_err(map_kb_error)?;
                        } else {
                            trace!("Class '{}' already exists, skipping", class.name);
                        }
                    }
                }
                kb.run().map_err(map_kb_error)?;
                Ok(())
            }
            .await;
            let _ = resp.send(result);
        }
        CoCoCommand::LoadClass { class_definition, resp } => {
            let result = async move {
                let class: Class = serde_json::from_str(&class_definition).map_err(|e| CoCoError::JsonParseError(format!("Failed to parse JSON for class definition: {:?}", e)))?;
                if let None = kb.get_class(&class.name) {
                    info!("Creating class '{}' with parents {:?}, static properties {:?}, and dynamic properties {:?}", class.name, class.parents, class.static_properties, class.dynamic_properties);
                    db.create_class(&class).await.map_err(map_db_error)?;
                    kb.create_class(class).map_err(map_kb_error)?;
                    kb.run().map_err(map_kb_error)?;
                } else {
                    trace!("Class '{}' already exists, skipping", class.name);
                }
                Ok(())
            }
            .await;
            let _ = resp.send(result);
        }
        CoCoCommand::GetClasses { resp } => {
            let _ = resp.send(kb.get_classes().into_iter().cloned().collect());
        }
        CoCoCommand::GetClass { name, resp } => {
            let _ = resp.send(kb.get_class(&name).cloned());
        }
        CoCoCommand::CreateClass { class, resp } => {
            let result = db.create_class(&class).await.map_err(map_db_error).and_then(|_| kb.create_class(class).and_then(|_| kb.run()).map_err(map_kb_error));
            let _ = resp.send(result);
        }
        CoCoCommand::LoadObjects { path, resp } => {
            let result = async move {
                let mut entries = fs::read_dir(&path).await.map_err(|e| CoCoError::DirectoryReadError(format!("Failed to read directory '{}': {:?}", path.display(), e)))?;
                while let Some(entry) = entries.next_entry().await.map_err(|e| CoCoError::FileReadError(format!("Failed to read entry in directory '{}': {:?}", path.display(), e)))? {
                    let path = entry.path();
                    if path.extension().and_then(|s| s.to_str()) == Some("json") {
                        let data = fs::read_to_string(&path).await.map_err(|e| CoCoError::FileReadError(format!("Failed to read file '{}': {:?}", path.display(), e)))?;
                        let object: Object = serde_json::from_str(&data).map_err(|e| CoCoError::JsonParseError(format!("Failed to parse JSON in file '{}': {:?}", path.display(), e)))?;
                        if let Some(id) = &object.id {
                            return Err(CoCoError::JsonParseError(format!("Object definition in file '{}' must not contain an 'id' field, but got id '{}'", path.display(), id)));
                        } else {
                            info!("Creating object with classes {:?}, properties {:?} and values {:?}", object.classes, object.properties, object.values);
                            db.create_object(&object).await.map_err(map_db_error)?;
                            kb.create_object(object).map_err(map_kb_error)?;
                        }
                    }
                }
                kb.run().map_err(map_kb_error)?;
                Ok(())
            }
            .await;
            let _ = resp.send(result);
        }
        CoCoCommand::LoadObject { object_definition, resp } => {
            let result = async move {
                let object: Object = serde_json::from_str(&object_definition).map_err(|e| CoCoError::JsonParseError(format!("Failed to parse JSON for object definition: {:?}", e)))?;
                if let Some(id) = &object.id {
                    return Err(CoCoError::JsonParseError(format!("Object definition must not contain an 'id' field, but got id '{}'", id)));
                } else {
                    info!("Creating object with classes {:?}, properties {:?} and values {:?}", object.classes, object.properties, object.values);
                    db.create_object(&object).await.map_err(map_db_error)?;
                    kb.create_object(object).map_err(map_kb_error)?;
                    kb.run().map_err(map_kb_error)?;
                }
                Ok(())
            }
            .await;
            let _ = resp.send(result);
        }
        CoCoCommand::GetObjects { resp } => {
            let _ = resp.send(kb.get_objects().into_iter().cloned().collect());
        }
        CoCoCommand::GetObject { id, resp } => {
            let _ = resp.send(kb.get_object(&id).cloned());
        }
        CoCoCommand::CreateObject { object, resp } => {
            trace!("Creating object with classes {:?} and properties {:?}", object.classes, object.properties);
            let result = db.create_object(&object).await.map_err(map_db_error).and_then(|id| {
                let object = Object { id: Some(id.clone()), ..object };
                kb.create_object(object).and_then(|_| kb.run()).map_err(map_kb_error).map(|_| id)
            });
            let _ = resp.send(result);
        }
        CoCoCommand::SetProperties { object_id, values, resp } => {
            trace!("Setting properties for object '{}': {:?}", object_id, values);
            let result = db.set_properties(&object_id, &values).await.map_err(map_db_error).and_then(|_| kb.set_properties(&object_id, values).and_then(|_| kb.run()).map_err(map_kb_error));
            let _ = resp.send(result);
        }
        CoCoCommand::AddData { object_id, values, date_time, resp } => {
            trace!("Adding data for object '{}': {:?} at {}", object_id, values, date_time);
            let result = db.add_data(&object_id, &values, &date_time).await.map_err(map_db_error).and_then(|_| kb.add_values(&object_id, values, date_time).and_then(|_| kb.run()).map_err(map_kb_error));
            let _ = resp.send(result);
        }
        CoCoCommand::GetData { object_id, start_time, end_time, resp } => {
            trace!("Getting data for object '{}', start_time: {:?}, end_time: {:?}", object_id, start_time, end_time);
            let result = db.get_data(&object_id, start_time.as_ref(), end_time.as_ref()).await.map_err(map_db_error);
            let _ = resp.send(result);
        }
        CoCoCommand::LoadRules { path, resp } => {
            info!("Loading rules from directory '{}'", path.display());
            let result = async move {
                let mut entries = fs::read_dir(&path).await.map_err(|e| CoCoError::DirectoryReadError(format!("Failed to read directory '{}': {:?}", path.display(), e)))?;
                while let Some(entry) = entries.next_entry().await.map_err(|e| CoCoError::FileReadError(format!("Failed to read entry in directory '{}': {:?}", path.display(), e)))? {
                    let path = entry.path();
                    if path.extension().and_then(|s| s.to_str()) == Some("json") {
                        let data = fs::read_to_string(&path).await.map_err(|e| CoCoError::FileReadError(format!("Failed to read file '{}': {:?}", path.display(), e)))?;
                        let rule: Rule = serde_json::from_str(&data).map_err(|e| CoCoError::JsonParseError(format!("Failed to parse JSON in file '{}': {:?}", path.display(), e)))?;
                        if let None = kb.get_rule(&rule.name).cloned() {
                            info!("Creating rule '{}'", rule.name);
                            db.create_rule(&rule).await.map_err(map_db_error)?;
                            kb.create_rule(rule).map_err(map_kb_error)?;
                        } else {
                            trace!("Rule '{}' already exists, skipping", rule.name);
                        }
                    }
                }
                kb.run().map_err(map_kb_error)?;
                Ok(())
            }
            .await;
            let _ = resp.send(result);
        }
        CoCoCommand::LoadRule { rule_definition, resp } => {
            let result = async move {
                let rule: Rule = serde_json::from_str(&rule_definition).map_err(|e| CoCoError::JsonParseError(format!("Failed to parse JSON for rule definition: {:?}", e)))?;
                if let None = kb.get_rule(&rule.name).cloned() {
                    info!("Creating rule '{}'", rule.name);
                    db.create_rule(&rule).await.map_err(map_db_error)?;
                    kb.create_rule(rule).map_err(map_kb_error)?;
                    kb.run().map_err(map_kb_error)?;
                } else {
                    trace!("Rule '{}' already exists, skipping", rule.name);
                }
                Ok(())
            }
            .await;
            let _ = resp.send(result);
        }
        CoCoCommand::GetRules { resp } => {
            let _ = resp.send(kb.get_rules().into_iter().cloned().collect());
        }
        CoCoCommand::GetRule { name, resp } => {
            let _ = resp.send(kb.get_rule(&name).cloned());
        }
        CoCoCommand::CreateRule { rule, resp } => {
            trace!("Creating rule '{}'", rule.name);
            let result = db.create_rule(&rule).await.map_err(map_db_error).and_then(|_| kb.create_rule(rule).and_then(|_| kb.run()).map_err(map_kb_error));
            let _ = resp.send(result);
        }
    }
}

async fn handle_kb_event(event: CoCoEvent, db: &Arc<dyn Database>, kb: &mut Box<dyn KnowledgeBase>, llm: &Option<Box<dyn LLM>>, fcm: &Option<Box<dyn Messaging>>, event_tx: &broadcast::Sender<CoCoEvent>) {
    match event {
        CoCoEvent::PendingClass(object_id, class_name) => {
            db.add_class(&object_id, &class_name).await.unwrap_or_else(|e| {
                eprintln!("Error adding class '{}' to object '{}' in database: {:?}", class_name, object_id, e);
            });
            if let Err(e) = kb.add_class(&object_id, &class_name) {
                eprintln!("Error adding class '{}' to object '{}' in knowledge base: {:?}", class_name, object_id, e);
            }
        }
        CoCoEvent::PendingValues(object_id, values, date_time) => {
            db.add_data(&object_id, &values, &date_time).await.unwrap_or_else(|e| {
                eprintln!("Error adding values to object '{}' in database: {:?}", object_id, e);
            });
            if let Err(e) = kb.add_values(&object_id, values, date_time) {
                eprintln!("Error adding values to object '{}' in knowledge base: {:?}", object_id, e);
            }
        }
        CoCoEvent::LLMPrompt(object_id, message) => {
            if let Some(llm) = llm {
                match llm.prompt(&message).await {
                    Ok(response) => {
                        let _ = event_tx.send(CoCoEvent::LLMResponse(object_id.clone(), response));
                    }
                    Err(e) => eprintln!("Error generating LLM response for object '{}': {:?}", object_id, e),
                }
            } else {
                eprintln!("Received LLM prompt for object '{}', but no LLM is configured", object_id);
            }
        }
        CoCoEvent::Message(object_id, title, message) => {
            if let Some(fcm) = fcm {
                let tokens = db.get_fcm_tokens(&object_id).await.unwrap_or_else(|e| {
                    eprintln!("Error fetching FCM tokens for object '{}' from database: {:?}", object_id, e);
                    vec![]
                });
                let failed_tokens = fcm.send_message(tokens, &title, &message).await;
                for token in failed_tokens.unwrap_or_default() {
                    db.remove_fcm_token(&object_id, &token).await.unwrap_or_else(|e| {
                        eprintln!("Error removing failed FCM token '{}' for object '{}' from database: {:?}", token, object_id, e);
                    });
                }
            } else {
                eprintln!("Received FCM message for object '{}', but no FCM client is configured: {} - {}", object_id, title, message);
            }
        }
        _ => {
            let _ = event_tx.send(event);
        }
    }
}

fn map_db_error(error: DatabaseError) -> CoCoError {
    match error {
        DatabaseError::ClassAlreadyExists(name) => CoCoError::ClassAlreadyExists(name),
        DatabaseError::ClassNotFound(name) => CoCoError::ClassNotFound(name),
        DatabaseError::ObjectAlreadyExists(id) => CoCoError::ObjectAlreadyExists(id),
        DatabaseError::ObjectNotFound(id) => CoCoError::ObjectNotFound(id),
        DatabaseError::RuleAlreadyExists(name) => CoCoError::RuleAlreadyExists(name),
        DatabaseError::RuleNotFound(name) => CoCoError::RuleNotFound(name),
        other => CoCoError::DatabaseError(format!("Database error: {:?}", other)),
    }
}

fn map_kb_error(error: KnowledgeBaseError) -> CoCoError {
    match error {
        KnowledgeBaseError::ClassAlreadyExists(name) => CoCoError::ClassAlreadyExists(name),
        KnowledgeBaseError::ClassNotFound(name) => CoCoError::ClassNotFound(name),
        KnowledgeBaseError::ObjectAlreadyExists(id) => CoCoError::ObjectAlreadyExists(id),
        KnowledgeBaseError::ObjectNotFound(id) => CoCoError::ObjectNotFound(id),
        KnowledgeBaseError::RuleAlreadyExists(name) => CoCoError::RuleAlreadyExists(name),
        KnowledgeBaseError::RuleNotFound(name) => CoCoError::RuleNotFound(name),
        other => CoCoError::KnowledgeBaseError(format!("Knowledge base error: {:?}", other)),
    }
}

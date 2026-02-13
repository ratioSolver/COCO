use chrono::{DateTime, Utc};
use tokio::sync::{broadcast, mpsc, oneshot};

use crate::{
    db::Database,
    kb::{KnowledgeBase, KnowledgeBaseError},
    model::{Class, CoCoEvent, Object, Property, Rule, Value},
};
use std::{
    collections::{HashMap, HashSet},
    fmt::Display,
    sync::Arc,
};

pub mod db;
pub mod kb;
pub mod model;

pub struct CoCo {
    kb_tx: mpsc::Sender<KbCommand>,
    event_tx: broadcast::Sender<CoCoEvent>,
}

enum KbCommand {
    InitData { classes: Vec<Class>, objects: Vec<Object>, rules: Vec<Rule>, resp: oneshot::Sender<()> },
    GetClasses { resp: oneshot::Sender<Vec<Class>> },
    GetClass { name: String, resp: oneshot::Sender<Option<Class>> },
    CreateClass { class: Class, resp: oneshot::Sender<Result<(), CoCoError>> },
    GetObjects { resp: oneshot::Sender<Vec<Object>> },
    GetObject { id: String, resp: oneshot::Sender<Option<Object>> },
    CreateObject { object: Object, resp: oneshot::Sender<Result<String, CoCoError>> },
    SetProperties { object_id: String, values: HashMap<String, Value>, resp: oneshot::Sender<Result<(), CoCoError>> },
    AddData { object_id: String, values: HashMap<String, Value>, date_time: DateTime<Utc>, resp: oneshot::Sender<Result<(), CoCoError>> },
    GetRules { resp: oneshot::Sender<Vec<Rule>> },
    GetRule { name: String, resp: oneshot::Sender<Option<Rule>> },
    CreateRule { rule: Rule, resp: oneshot::Sender<Result<(), CoCoError>> },
}

#[derive(Debug)]
pub enum CoCoError {
    ConnectionError(String),
    ClassNotFound(String),
    ClassAlreadyExists(String),
}

impl Display for CoCoError {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        match self {
            CoCoError::ConnectionError(msg) => write!(f, "Connection error: {}", msg),
            CoCoError::ClassNotFound(class) => write!(f, "Class not found: {}", class),
            CoCoError::ClassAlreadyExists(class) => write!(f, "Class already exists: {}", class),
        }
    }
}

impl CoCo {
    pub async fn new(db: Arc<dyn Database>, mut kb: Box<dyn KnowledgeBase>) -> Self {
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
                        handle_kb_event(event, &db_for_task, &mut kb, &event_tx_task).await;
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
        let _ = kb_tx.send(KbCommand::InitData { classes, objects, rules, resp: resp_tx }).await;
        let _ = resp_rx.await;

        CoCo { kb_tx, event_tx }
    }

    pub fn get_event_sender(&self) -> broadcast::Sender<CoCoEvent> {
        self.event_tx.clone()
    }

    pub async fn get_classes(&self) -> Vec<Class> {
        let (resp_tx, resp_rx) = oneshot::channel();
        let _ = self.kb_tx.send(KbCommand::GetClasses { resp: resp_tx }).await;
        resp_rx.await.unwrap_or_default()
    }

    pub async fn get_class(&self, name: &str) -> Option<Class> {
        let (resp_tx, resp_rx) = oneshot::channel();
        let _ = self.kb_tx.send(KbCommand::GetClass { name: name.to_string(), resp: resp_tx }).await;
        resp_rx.await.unwrap_or(None)
    }

    pub async fn create_new_class(&self, name: &str, parents: Option<HashSet<String>>, static_properties: Option<HashMap<String, Property>>, dynamic_properties: Option<HashMap<String, Property>>) -> Result<(), CoCoError> {
        let class = Class { name: name.to_string(), parents, static_properties, dynamic_properties };
        self.create_class(class).await
    }

    pub async fn create_class(&self, class: Class) -> Result<(), CoCoError> {
        let (resp_tx, resp_rx) = oneshot::channel();
        let _ = self.kb_tx.send(KbCommand::CreateClass { class, resp: resp_tx }).await;
        resp_rx.await.unwrap_or_else(|_| Err(CoCoError::ConnectionError("KB task closed".to_string())))
    }

    pub async fn get_objects(&self) -> Vec<Object> {
        let (resp_tx, resp_rx) = oneshot::channel();
        let _ = self.kb_tx.send(KbCommand::GetObjects { resp: resp_tx }).await;
        resp_rx.await.unwrap_or_default()
    }

    pub async fn get_object(&self, id: &str) -> Option<Object> {
        let (resp_tx, resp_rx) = oneshot::channel();
        let _ = self.kb_tx.send(KbCommand::GetObject { id: id.to_string(), resp: resp_tx }).await;
        resp_rx.await.unwrap_or(None)
    }

    pub async fn create_new_object(&self, classes: HashSet<String>, properties: Option<HashMap<String, Value>>, values: Option<HashMap<String, (Value, DateTime<Utc>)>>) -> Result<String, CoCoError> {
        self.create_object(Object { id: None, classes, properties, values }).await
    }

    pub async fn create_object(&self, object: Object) -> Result<String, CoCoError> {
        let (resp_tx, resp_rx) = oneshot::channel();
        let _ = self.kb_tx.send(KbCommand::CreateObject { object, resp: resp_tx }).await;
        resp_rx.await.unwrap_or_else(|_| Err(CoCoError::ConnectionError("KB task closed".to_string())))
    }

    pub async fn set_properties(&self, object_id: &str, values: HashMap<String, Value>) -> Result<(), CoCoError> {
        let (resp_tx, resp_rx) = oneshot::channel();
        let _ = self.kb_tx.send(KbCommand::SetProperties { object_id: object_id.to_string(), values, resp: resp_tx }).await;
        resp_rx.await.unwrap_or_else(|_| Err(CoCoError::ConnectionError("KB task closed".to_string())))
    }

    pub async fn add_data(&self, object_id: &str, values: HashMap<String, Value>, date_time: DateTime<Utc>) -> Result<(), CoCoError> {
        let (resp_tx, resp_rx) = oneshot::channel();
        let _ = self.kb_tx.send(KbCommand::AddData { object_id: object_id.to_string(), values, date_time, resp: resp_tx }).await;
        resp_rx.await.unwrap_or_else(|_| Err(CoCoError::ConnectionError("KB task closed".to_string())))
    }

    pub async fn get_rules(&self) -> Vec<Rule> {
        let (resp_tx, resp_rx) = oneshot::channel();
        let _ = self.kb_tx.send(KbCommand::GetRules { resp: resp_tx }).await;
        resp_rx.await.unwrap_or_default()
    }

    pub async fn get_rule(&self, name: &str) -> Option<Rule> {
        let (resp_tx, resp_rx) = oneshot::channel();
        let _ = self.kb_tx.send(KbCommand::GetRule { name: name.to_string(), resp: resp_tx }).await;
        resp_rx.await.unwrap_or(None)
    }

    pub async fn create_new_rule(&self, name: &str, content: &str) -> Result<(), CoCoError> {
        self.create_rule(Rule { name: name.to_string(), content: content.to_string() }).await
    }

    pub async fn create_rule(&self, rule: Rule) -> Result<(), CoCoError> {
        let (resp_tx, resp_rx) = oneshot::channel();
        let _ = self.kb_tx.send(KbCommand::CreateRule { rule, resp: resp_tx }).await;
        resp_rx.await.unwrap_or_else(|_| Err(CoCoError::ConnectionError("KB task closed".to_string())))
    }
}

async fn handle_command(cmd: KbCommand, db: &Arc<dyn Database>, kb: &mut Box<dyn KnowledgeBase>) {
    match cmd {
        KbCommand::InitData { classes, objects, rules, resp } => {
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
            let _ = resp.send(());
        }
        KbCommand::GetClasses { resp } => {
            let _ = resp.send(kb.get_classes());
        }
        KbCommand::GetClass { name, resp } => {
            let _ = resp.send(kb.get_class(&name));
        }
        KbCommand::CreateClass { class, resp } => {
            let result = db
                .create_class(&class)
                .await
                .map_err(|e| {
                    eprintln!("Error creating class '{}' in database: {:?}", class.name, e);
                    CoCoError::ConnectionError(format!("Failed to create class '{}'", class.name))
                })
                .and_then(|_| kb.create_class(class).map_err(map_kb_error));
            let _ = resp.send(result);
        }
        KbCommand::GetObjects { resp } => {
            let _ = resp.send(kb.get_objects());
        }
        KbCommand::GetObject { id, resp } => {
            let _ = resp.send(kb.get_object(&id));
        }
        KbCommand::CreateObject { object, resp } => {
            let result = db
                .create_object(&object)
                .await
                .map_err(|e| {
                    eprintln!("Error creating object in database: {:?}", e);
                    CoCoError::ConnectionError("Failed to create object".to_string())
                })
                .and_then(|id| {
                    let object = Object { id: Some(id.clone()), ..object };
                    kb.create_object(object).map_err(map_kb_error).map(|_| id)
                });
            let _ = resp.send(result);
        }
        KbCommand::SetProperties { object_id, values, resp } => {
            let result = db
                .set_properties(&object_id, &values)
                .await
                .map_err(|e| {
                    eprintln!("Error setting properties for object '{}' in database: {:?}", object_id, e);
                    CoCoError::ConnectionError(format!("Failed to set properties for object '{}'", object_id))
                })
                .and_then(|_| kb.set_properties(&object_id, values).map_err(map_kb_error));
            let _ = resp.send(result);
        }
        KbCommand::AddData { object_id, values, date_time, resp } => {
            let result = db
                .add_data(&object_id, &values, &date_time)
                .await
                .map_err(|e| {
                    eprintln!("Error adding data to object '{}' in database: {:?}", object_id, e);
                    CoCoError::ConnectionError(format!("Failed to add data to object '{}'", object_id))
                })
                .and_then(|_| kb.add_values(&object_id, values, date_time).map_err(map_kb_error));
            let _ = resp.send(result);
        }
        KbCommand::GetRules { resp } => {
            let _ = resp.send(kb.get_rules());
        }
        KbCommand::GetRule { name, resp } => {
            let _ = resp.send(kb.get_rule(&name));
        }
        KbCommand::CreateRule { rule, resp } => {
            let result = db
                .create_rule(&rule)
                .await
                .map_err(|e| {
                    eprintln!("Error creating rule '{}' in database: {:?}", rule.name, e);
                    CoCoError::ConnectionError(format!("Failed to create rule '{}'", rule.name))
                })
                .and_then(|_| kb.create_rule(rule).map_err(map_kb_error));
            let _ = resp.send(result);
        }
    }
}

async fn handle_kb_event(event: CoCoEvent, db: &Arc<dyn Database>, kb: &mut Box<dyn KnowledgeBase>, event_tx: &broadcast::Sender<CoCoEvent>) {
    match event {
        CoCoEvent::PendingClass(object, class) => {
            db.add_class(&object, &class).await.unwrap_or_else(|e| {
                eprintln!("Error adding class '{}' to object '{}' in database: {:?}", class, object, e);
            });
            if let Err(e) = kb.add_class(&object, &class) {
                eprintln!("Error adding class '{}' to object '{}' in knowledge base: {:?}", class, object, e);
            }
        }
        CoCoEvent::PendingValues(object, values, date_time) => {
            db.add_data(&object, &values, &date_time).await.unwrap_or_else(|e| {
                eprintln!("Error adding values to object '{}' in database: {:?}", object, e);
            });
            if let Err(e) = kb.add_values(&object, values, date_time) {
                eprintln!("Error adding values to object '{}' in knowledge base: {:?}", object, e);
            }
        }
        _ => {
            let _ = event_tx.send(event);
        }
    }
}

fn map_kb_error(error: KnowledgeBaseError) -> CoCoError {
    match error {
        KnowledgeBaseError::ClassAlreadyExists(name) => CoCoError::ClassAlreadyExists(name),
        KnowledgeBaseError::ClassNotFound(name) => CoCoError::ClassNotFound(name),
        other => CoCoError::ConnectionError(format!("Knowledge base error: {:?}", other)),
    }
}

use crate::{
    db::Database,
    kb::{KnowledgeBase, KnowledgeBaseError},
    llm::LLM,
    model::{Class, CoCoEvent, Object, Property, Rule, Value},
    msg::Messaging,
};
use chrono::{DateTime, Utc};
use std::{
    collections::{HashMap, HashSet},
    sync::Arc,
};
use tokio::sync::{broadcast, mpsc, oneshot};

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
    kb_tx: mpsc::Sender<KbCommand>,
    event_tx: broadcast::Sender<CoCoEvent>,
}

pub trait CoCoState: Clone + Send + Sync + 'static {
    fn coco(&self) -> Arc<CoCo>;
}

#[derive(Clone, Debug)]
pub enum CoCoError {
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
        resp_rx.await.unwrap_or_else(|_| Err(CoCoError::DatabaseError("KB task closed".to_string())))
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
        resp_rx.await.unwrap_or_else(|_| Err(CoCoError::DatabaseError("KB task closed".to_string())))
    }

    pub async fn set_properties(&self, object_id: &str, values: HashMap<String, Value>) -> Result<(), CoCoError> {
        let (resp_tx, resp_rx) = oneshot::channel();
        let _ = self.kb_tx.send(KbCommand::SetProperties { object_id: object_id.to_string(), values, resp: resp_tx }).await;
        resp_rx.await.unwrap_or_else(|_| Err(CoCoError::DatabaseError("KB task closed".to_string())))
    }

    pub async fn add_data(&self, object_id: &str, values: HashMap<String, Value>, date_time: DateTime<Utc>) -> Result<(), CoCoError> {
        let (resp_tx, resp_rx) = oneshot::channel();
        let _ = self.kb_tx.send(KbCommand::AddData { object_id: object_id.to_string(), values, date_time, resp: resp_tx }).await;
        resp_rx.await.unwrap_or_else(|_| Err(CoCoError::DatabaseError("KB task closed".to_string())))
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
        resp_rx.await.unwrap_or_else(|_| Err(CoCoError::DatabaseError("KB task closed".to_string())))
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
            let _ = resp.send(kb.get_classes().into_iter().cloned().collect());
        }
        KbCommand::GetClass { name, resp } => {
            let _ = resp.send(kb.get_class(&name).cloned());
        }
        KbCommand::CreateClass { class, resp } => {
            let result = db
                .create_class(&class)
                .await
                .map_err(|e| {
                    eprintln!("Error creating class '{}' in database: {:?}", class.name, e);
                    CoCoError::DatabaseError(format!("Failed to create class '{}'", class.name))
                })
                .and_then(|_| kb.create_class(class).map_err(map_kb_error));
            let _ = resp.send(result);
        }
        KbCommand::GetObjects { resp } => {
            let _ = resp.send(kb.get_objects().into_iter().cloned().collect());
        }
        KbCommand::GetObject { id, resp } => {
            let _ = resp.send(kb.get_object(&id).cloned());
        }
        KbCommand::CreateObject { object, resp } => {
            let result = db
                .create_object(&object)
                .await
                .map_err(|e| {
                    eprintln!("Error creating object in database: {:?}", e);
                    CoCoError::DatabaseError("Failed to create object".to_string())
                })
                .and_then(|id| {
                    let object = Object { id: Some(id.clone()), ..object };
                    kb.create_object(object).and_then(|_| kb.run()).map_err(map_kb_error).map(|_| id)
                });
            let _ = resp.send(result);
        }
        KbCommand::SetProperties { object_id, values, resp } => {
            let result = db
                .set_properties(&object_id, &values)
                .await
                .map_err(|e| {
                    eprintln!("Error setting properties for object '{}' in database: {:?}", object_id, e);
                    CoCoError::DatabaseError(format!("Failed to set properties for object '{}'", object_id))
                })
                .and_then(|_| kb.set_properties(&object_id, values).and_then(|_| kb.run()).map_err(map_kb_error));
            let _ = resp.send(result);
        }
        KbCommand::AddData { object_id, values, date_time, resp } => {
            let result = db
                .add_data(&object_id, &values, &date_time)
                .await
                .map_err(|e| {
                    eprintln!("Error adding data to object '{}' in database: {:?}", object_id, e);
                    CoCoError::DatabaseError(format!("Failed to add data to object '{}'", object_id))
                })
                .and_then(|_| kb.add_values(&object_id, values, date_time).and_then(|_| kb.run()).map_err(map_kb_error));
            let _ = resp.send(result);
        }
        KbCommand::GetRules { resp } => {
            let _ = resp.send(kb.get_rules().into_iter().cloned().collect());
        }
        KbCommand::GetRule { name, resp } => {
            let _ = resp.send(kb.get_rule(&name).cloned());
        }
        KbCommand::CreateRule { rule, resp } => {
            let result = db
                .create_rule(&rule)
                .await
                .map_err(|e| {
                    eprintln!("Error creating rule '{}' in database: {:?}", rule.name, e);
                    CoCoError::DatabaseError(format!("Failed to create rule '{}'", rule.name))
                })
                .and_then(|_| kb.create_rule(rule).and_then(|_| kb.run()).map_err(map_kb_error));
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

fn map_kb_error(error: KnowledgeBaseError) -> CoCoError {
    match error {
        KnowledgeBaseError::ClassAlreadyExists(name) => CoCoError::ClassAlreadyExists(format!("Class '{}' already exists", name)),
        KnowledgeBaseError::ClassNotFound(name) => CoCoError::ClassNotFound(format!("Class '{}' not found", name)),
        KnowledgeBaseError::ObjectAlreadyExists(id) => CoCoError::ObjectAlreadyExists(format!("Object with ID '{}' already exists", id)),
        KnowledgeBaseError::ObjectNotFound(id) => CoCoError::ObjectNotFound(format!("Object with ID '{}' not found", id)),
        KnowledgeBaseError::RuleAlreadyExists(name) => CoCoError::RuleAlreadyExists(format!("Rule '{}' already exists", name)),
        KnowledgeBaseError::RuleNotFound(name) => CoCoError::RuleNotFound(format!("Rule '{}' not found", name)),
        other => CoCoError::KnowledgeBaseError(format!("Knowledge base error: {:?}", other)),
    }
}

use crate::{
    db::{Database, DatabaseError},
    kb::{KnowledgeBase, KnowledgeBaseError},
    llm::LLM,
    model::{Class, CoCoError, CoCoEvent, Object, Property, Rule, TimedValue, Value},
    msg::Messaging,
};
use chrono::{DateTime, Utc};
use std::{
    collections::{HashMap, HashSet},
    fs,
    path::Path,
    sync::Arc,
};
use tracing::{error, info, trace};

pub mod db;
pub mod kb;
pub mod llm;
pub mod model;
pub mod msg;
#[cfg(feature = "server")]
pub mod server;

pub type Callback = Arc<dyn Fn(CoCoEvent) + Send + Sync + 'static>;

pub struct CoCo<KB: KnowledgeBase> {
    kb: KB,
    db: Arc<dyn Database>,
    callback: Option<Callback>,
}

impl<KB: KnowledgeBase> CoCo<KB> {
    pub async fn new(kb: KB, db: Arc<dyn Database>, llm: Option<Arc<dyn LLM>>, fcm: Option<Arc<dyn Messaging>>) -> Self {
        let mut coco = Self { kb, db: db.clone(), callback: None };

        let cb_callback = coco.callback.clone();
        let cb_llm = llm.clone();
        coco.kb.set_callback(move |query| match query {
            kb::KnowledgeBaseEvent::AddedClass(object_id, class) => {
                let db = db.clone();
                let cb_callback = cb_callback.clone();
                tokio::spawn(async move {
                    match db.add_class(&object_id, &class).await {
                        Ok(_) => {
                            if let Some(cb) = cb_callback.as_ref() {
                                cb(CoCoEvent::AddedClass(object_id.clone(), class.clone()));
                            }
                        }
                        Err(e) => error!("Failed to add class to database: {}", e),
                    }
                });
            }
            kb::KnowledgeBaseEvent::UpdatedProperties(object_id, properties) => {
                let db = db.clone();
                let cb_callback = cb_callback.clone();
                tokio::spawn(async move {
                    match db.set_properties(&object_id, &properties).await {
                        Ok(_) => {
                            if let Some(cb) = cb_callback.as_ref() {
                                cb(CoCoEvent::UpdatedProperties(object_id.clone(), properties.clone()));
                            }
                        }
                        Err(e) => error!("Failed to update properties in database: {}", e),
                    }
                });
            }
            kb::KnowledgeBaseEvent::AddedValues(object_id, values, date_time) => {
                let db = db.clone();
                let cb_callback = cb_callback.clone();
                tokio::spawn(async move {
                    match db.add_data(&object_id, &values, &date_time).await {
                        Ok(_) => {
                            if let Some(cb) = cb_callback.as_ref() {
                                cb(CoCoEvent::AddedValues(object_id.clone(), values.clone(), date_time));
                            }
                        }
                        Err(e) => error!("Failed to add values to database: {}", e),
                    }
                });
            }
            kb::KnowledgeBaseEvent::LLMPrompt(object_id, prompt) => {
                let llm = cb_llm.clone();
                tokio::spawn(async move {
                    if let Some(llm) = llm.as_ref() {
                        if llm.async_prompt(&object_id, &prompt).await.is_err() {
                            error!("Failed to get LLM response");
                        }
                    } else {
                        error!("No LLM configured");
                    }
                });
            }
            kb::KnowledgeBaseEvent::Message(object_id, title, message) => {
                let fcm = fcm.clone();
                tokio::spawn(async move {
                    if let Some(fcm) = fcm.as_ref() {
                        if fcm.send_message(vec![object_id], &title, &message).await.is_err() {
                            error!("Failed to send message");
                        }
                    } else {
                        error!("No messaging service configured");
                    }
                });
            }
        });

        coco.kb.set_llm_callback(move |prompt| {
            if let Some(llm) = llm.as_ref() {
                let response = tokio::runtime::Handle::current().block_on(llm.prompt(&prompt));
                match response {
                    Ok(result) => result,
                    Err(e) => {
                        error!("LLM error: {:?}", e);
                        "LLM error".to_string()
                    }
                }
            } else {
                "No LLM configured".to_string()
            }
        });

        info!("Loading classes, objects, and rules from database into knowledge base");
        let classes = coco.db.get_classes().await.unwrap_or_else(|e| {
            error!("Error fetching classes from database: {:?}", e);
            vec![]
        });
        for class in classes {
            coco.kb.create_class(class).unwrap_or_else(|e| {
                error!("Error creating class in knowledge base: {:?}", e);
            });
        }

        let objects = coco.db.get_objects().await.unwrap_or_else(|e| {
            error!("Error fetching objects from database: {:?}", e);
            vec![]
        });
        for object in objects {
            coco.kb.create_object(object).unwrap_or_else(|e| {
                error!("Error creating object in knowledge base: {:?}", e);
            });
        }

        let rules = coco.db.get_rules().await.unwrap_or_else(|e| {
            error!("Error fetching rules from database: {:?}", e);
            vec![]
        });
        for rule in rules {
            coco.kb.create_rule(rule).unwrap_or_else(|e| {
                error!("Error creating rule in knowledge base: {:?}", e);
            });
        }

        coco
    }

    pub async fn load_classes<P: AsRef<Path>>(&mut self, path: P) -> Result<(), CoCoError> {
        info!("Loading classes from directory '{}'", path.as_ref().display());
        let entries = fs::read_dir(path).map_err(|e| CoCoError::FileReadError(e.to_string()))?;
        for entry in entries {
            let entry = entry.map_err(|e| CoCoError::FileReadError(e.to_string()))?;
            let path = entry.path();
            if path.is_file() {
                let content = fs::read_to_string(&path).map_err(|e| CoCoError::FileReadError(e.to_string()))?;
                let class: model::Class = serde_json::from_str(&content).map_err(|e| CoCoError::JsonParseError(e.to_string()))?;
                if self.kb.get_class(&class.name).is_none() {
                    let class_name = class.name.clone();
                    info!("Creating class '{}' with parents {:?}, static properties {:?}, and dynamic properties {:?}", class_name, class.parents, class.static_properties, class.dynamic_properties);
                    self.db.create_class(&class).await.map_err(map_db_error)?;
                    self.kb.create_class(class).map_err(map_kb_error)?;
                    self.notify(CoCoEvent::ClassCreated(class_name));
                } else {
                    trace!("Class '{}' already exists, skipping '{}'", class.name, path.display());
                }
            }
        }
        Ok(())
    }

    pub async fn load_class(&mut self, class_definition: &str) -> Result<(), CoCoError> {
        let class: model::Class = serde_json::from_str(class_definition).map_err(|e| CoCoError::JsonParseError(e.to_string()))?;
        if self.kb.get_class(&class.name).is_none() {
            let class_name = class.name.clone();
            info!("Creating class '{}' with parents {:?}, static properties {:?}, and dynamic properties {:?}", class_name, class.parents, class.static_properties, class.dynamic_properties);
            self.db.create_class(&class).await.map_err(map_db_error)?;
            self.kb.create_class(class).map_err(map_kb_error)?;
            self.notify(CoCoEvent::ClassCreated(class_name));
            Ok(())
        } else {
            trace!("Class '{}' already exists, skipping", class.name);
            Err(CoCoError::ClassAlreadyExists(class.name))
        }
    }

    pub async fn get_classes(&self) -> Vec<Class> {
        self.kb.get_classes().into_iter().cloned().collect()
    }

    pub async fn get_class(&self, name: &str) -> Option<Class> {
        self.kb.get_class(name).cloned()
    }

    pub async fn new_class(&mut self, name: &str, parents: Option<HashSet<String>>, static_properties: Option<HashMap<String, Property>>, dynamic_properties: Option<HashMap<String, Property>>) -> Result<(), CoCoError> {
        let class = Class { name: name.to_owned(), parents, static_properties, dynamic_properties };
        self.create_class(class).await
    }

    pub async fn create_class(&mut self, class: Class) -> Result<(), CoCoError> {
        self.db.create_class(&class).await.map_err(map_db_error)?;
        self.kb.create_class(class.clone()).map_err(map_kb_error)?;
        self.notify(CoCoEvent::ClassCreated(class.name));
        Ok(())
    }

    pub async fn load_objects<P: AsRef<Path>>(&mut self, path: P) -> Result<(), CoCoError> {
        info!("Loading objects from directory '{}'", path.as_ref().display());
        let entries = fs::read_dir(path).map_err(|e| CoCoError::FileReadError(e.to_string()))?;
        for entry in entries {
            let entry = entry.map_err(|e| CoCoError::FileReadError(e.to_string()))?;
            let path = entry.path();
            if path.is_file() {
                let content = fs::read_to_string(&path).map_err(|e| CoCoError::FileReadError(e.to_string()))?;
                let object: model::Object = serde_json::from_str(&content).map_err(|e| CoCoError::JsonParseError(e.to_string()))?;
                if let Some(id) = &object.id {
                    return Err(CoCoError::JsonParseError(format!("Object definition in file '{}' must not contain an 'id' field, but got id '{}'", path.display(), id)));
                } else {
                    info!("Creating object with classes {:?}, properties {:?}, and data {:?}", object.classes, object.properties, object.values);
                    let object_id = self.db.create_object(&object).await.map_err(map_db_error)?;
                    self.kb.create_object(object.clone()).map_err(map_kb_error)?;
                    self.notify(CoCoEvent::ObjectCreated(object_id));
                }
            }
        }
        Ok(())
    }

    pub async fn load_object(&mut self, object_definition: &str) -> Result<(), CoCoError> {
        let object: model::Object = serde_json::from_str(object_definition).map_err(|e| CoCoError::JsonParseError(e.to_string()))?;
        if let Some(id) = &object.id {
            Err(CoCoError::JsonParseError(format!("Object definition must not contain an 'id' field, but got id '{}'", id)))
        } else {
            info!("Creating object with classes {:?}, properties {:?}, and data {:?}", object.classes, object.properties, object.values);
            let object_id = self.db.create_object(&object).await.map_err(map_db_error)?;
            self.kb.create_object(object.clone()).map_err(map_kb_error)?;
            self.notify(CoCoEvent::ObjectCreated(object_id));
            Ok(())
        }
    }

    pub async fn get_objects(&self) -> Vec<Object> {
        self.kb.get_objects().into_iter().cloned().collect()
    }

    pub async fn get_object(&self, id: &str) -> Option<Object> {
        self.kb.get_object(id).cloned()
    }

    pub async fn new_object(&mut self, classes: HashSet<String>, properties: Option<HashMap<String, Value>>, values: Option<HashMap<String, TimedValue>>) -> Result<String, CoCoError> {
        let object = Object { id: None, classes, properties, values };
        self.create_object(object).await
    }

    pub async fn create_object(&mut self, object: Object) -> Result<String, CoCoError> {
        let object_id = self.db.create_object(&object).await.map_err(map_db_error)?;
        self.kb.create_object(Object { id: Some(object_id.clone()), ..object }).map_err(map_kb_error)?;
        self.notify(CoCoEvent::ObjectCreated(object_id.clone()));
        Ok(object_id)
    }

    pub async fn set_properties(&mut self, object_id: &str, values: HashMap<String, Value>) -> Result<(), CoCoError> {
        self.db.set_properties(object_id, &values).await.map_err(map_db_error)?;
        self.kb.set_properties(object_id, values.clone()).map_err(map_kb_error)?;
        self.notify(CoCoEvent::UpdatedProperties(object_id.to_owned(), values));
        Ok(())
    }

    pub async fn add_data(&mut self, object_id: &str, values: HashMap<String, Value>, date_time: DateTime<Utc>) -> Result<(), CoCoError> {
        self.db.add_data(object_id, &values, &date_time).await.map_err(map_db_error)?;
        self.kb.add_values(object_id, values.clone(), date_time).map_err(map_kb_error)?;
        self.notify(CoCoEvent::AddedValues(object_id.to_owned(), values, date_time));
        Ok(())
    }

    pub async fn get_data(&self, object_id: &str, start_time: Option<DateTime<Utc>>, end_time: Option<DateTime<Utc>>) -> Result<Vec<(HashMap<String, Value>, DateTime<Utc>)>, CoCoError> {
        self.db.get_data(object_id, start_time.as_ref(), end_time.as_ref()).await.map_err(map_db_error)
    }

    pub async fn load_rules<P: AsRef<Path>>(&mut self, path: P) -> Result<(), CoCoError> {
        info!("Loading rules from directory '{}'", path.as_ref().display());
        let entries = fs::read_dir(path).map_err(|e| CoCoError::FileReadError(e.to_string()))?;
        for entry in entries {
            let entry = entry.map_err(|e| CoCoError::FileReadError(e.to_string()))?;
            let path = entry.path();
            if path.is_file() {
                let content = fs::read_to_string(&path).map_err(|e| CoCoError::FileReadError(e.to_string()))?;
                let rule: model::Rule = serde_json::from_str(&content).map_err(|e| CoCoError::JsonParseError(e.to_string()))?;
                if self.kb.get_rule(&rule.name).is_none() {
                    let rule_name = rule.name.clone();
                    info!("Creating rule '{}' with content {:?}", rule_name, rule.content);
                    self.db.create_rule(&rule).await.map_err(map_db_error)?;
                    self.kb.create_rule(rule.clone()).map_err(map_kb_error)?;
                    self.notify(CoCoEvent::RuleCreated(rule_name));
                } else {
                    trace!("Rule '{}' already exists, skipping '{}'", rule.name, path.display());
                }
            }
        }
        Ok(())
    }

    pub async fn load_rule(&mut self, rule_definition: &str) -> Result<(), CoCoError> {
        let rule: model::Rule = serde_json::from_str(rule_definition).map_err(|e| CoCoError::JsonParseError(e.to_string()))?;
        if self.kb.get_rule(&rule.name).is_none() {
            let rule_name = rule.name.clone();
            info!("Creating rule '{}' with content {:?}", rule_name, rule.content);
            self.db.create_rule(&rule).await.map_err(map_db_error)?;
            self.kb.create_rule(rule.clone()).map_err(map_kb_error)?;
            self.notify(CoCoEvent::RuleCreated(rule_name));
            Ok(())
        } else {
            trace!("Rule '{}' already exists, skipping", rule.name);
            Err(CoCoError::RuleAlreadyExists(rule.name))
        }
    }

    pub async fn get_rules(&self) -> Vec<Rule> {
        self.kb.get_rules().into_iter().cloned().collect()
    }

    pub async fn get_rule(&self, name: &str) -> Option<Rule> {
        self.kb.get_rule(name).cloned()
    }

    pub async fn new_rule(&mut self, name: &str, content: &str) -> Result<(), CoCoError> {
        let rule = Rule { name: name.to_owned(), content: content.to_owned() };
        self.create_rule(rule).await
    }

    pub async fn create_rule(&mut self, rule: Rule) -> Result<(), CoCoError> {
        self.db.create_rule(&rule).await.map_err(map_db_error)?;
        self.kb.create_rule(rule.clone()).map_err(map_kb_error)?;
        self.notify(CoCoEvent::RuleCreated(rule.name));
        Ok(())
    }

    fn notify(&self, event: CoCoEvent) {
        if let Some(cb) = self.callback.as_ref() {
            cb(event);
        }
    }

    pub fn set_callback(&mut self, callback: Callback) {
        self.callback.replace(callback);
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

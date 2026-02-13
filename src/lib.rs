use chrono::{DateTime, Utc};
use tokio::sync::broadcast;

use crate::{
    db::Database,
    kb::KnowledgeBase,
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
    db: Arc<dyn Database>,
    kb: Arc<dyn KnowledgeBase>,
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
    pub async fn new(db: Arc<dyn Database>, kb: Arc<dyn KnowledgeBase>) -> Self {
        let mut coco = CoCo { db: db.clone(), kb: kb.clone() };
        coco.add_classes(db.get_classes().await.unwrap_or_else(|e| {
            eprintln!("Error fetching classes from database: {:?}", e);
            vec![]
        }));
        coco.add_objects(db.get_objects().await.unwrap_or_else(|e| {
            eprintln!("Error fetching objects from database: {:?}", e);
            vec![]
        }));
        coco.add_rules(db.get_rules().await.unwrap_or_else(|e| {
            eprintln!("Error fetching rules from database: {:?}", e);
            vec![]
        }));

        let mut receiver = kb.get_event_sender().subscribe();
        let mut ac_kb = coco.kb.clone();
        let mut ac_db = coco.db.clone();
        let mut av_kb = coco.kb.clone();
        let mut av_db = coco.db.clone();
        tokio::spawn(async move {
            while let Ok(event) = receiver.recv().await {
                match event {
                    CoCoEvent::PendingClass(object, class) => {
                        let db = Arc::get_mut(&mut ac_db).unwrap();
                        db.add_class(&object, &class).await.unwrap_or_else(|e| {
                            eprintln!("Error adding class '{}' to object '{}' in database: {:?}", class, object, e);
                        });
                        let kb = Arc::get_mut(&mut ac_kb).unwrap();
                        kb.add_class(&object, &class).unwrap_or_else(|e| {
                            eprintln!("Error adding class '{}' to object '{}' in knowledge base: {:?}", class, object, e);
                        });
                    }
                    CoCoEvent::PendingValues(object, values, date_time) => {
                        let db = Arc::get_mut(&mut av_db).unwrap();
                        db.add_data(&object, &values, &date_time).await.unwrap_or_else(|e| {
                            eprintln!("Error adding values to object '{}' in database: {:?}", object, e);
                        });
                        let kb = Arc::get_mut(&mut av_kb).unwrap();
                        kb.add_values(&object, values, date_time).unwrap_or_else(|e| {
                            eprintln!("Error adding values to object '{}' in knowledge base: {:?}", object, e);
                        });
                    }
                    _ => {}
                }
            }
        });

        coco
    }

    pub fn get_event_sender(&self) -> broadcast::Sender<CoCoEvent> {
        self.kb.get_event_sender()
    }

    pub fn get_classes(&self) -> Vec<Class> {
        self.kb.get_classes()
    }

    pub fn get_class(&self, name: &str) -> Option<Class> {
        self.kb.get_class(name)
    }

    pub async fn create_new_class(&mut self, name: &str, parents: Option<HashSet<String>>, static_properties: Option<HashMap<String, Property>>, dynamic_properties: Option<HashMap<String, Property>>) -> Result<(), CoCoError> {
        let class = Class { name: name.to_string(), parents, static_properties, dynamic_properties };
        self.db.create_class(&class).await.map_err(|e| {
            eprintln!("Error creating class '{}' in database: {:?}", name, e);
            CoCoError::ConnectionError(format!("Failed to create class '{}'", name))
        })?;
        self.add_classes(vec![class]);
        Ok(())
    }

    pub async fn create_class(&mut self, class: Class) -> Result<(), CoCoError> {
        self.db.create_class(&class).await.expect("Failed to create class in database");
        self.add_classes(vec![class]);
        Ok(())
    }

    pub fn get_objects(&self) -> Vec<Object> {
        self.kb.get_objects()
    }

    pub fn get_object(&self, id: &str) -> Option<Object> {
        self.kb.get_object(id)
    }

    pub async fn create_new_object(&mut self, classes: HashSet<String>, properties: Option<HashMap<String, Value>>, values: Option<HashMap<String, (Value, DateTime<Utc>)>>) -> Result<String, CoCoError> {
        self.create_object(Object { id: None, classes, properties, values }).await
    }

    pub async fn create_object(&mut self, object: Object) -> Result<String, CoCoError> {
        let id = self.db.create_object(&object).await.map_err(|e| {
            eprintln!("Error creating object in database: {:?}", e);
            CoCoError::ConnectionError("Failed to create object".to_string())
        })?;
        let object = Object { id: Some(id.clone()), ..object };
        self.add_objects(vec![object]);
        Ok(id)
    }

    pub async fn set_properties(&mut self, object_id: &str, values: HashMap<String, Value>) -> Result<(), CoCoError> {
        self.db.set_properties(object_id, &values).await.map_err(|e| {
            eprintln!("Error setting properties for object '{}' in database: {:?}", object_id, e);
            CoCoError::ConnectionError(format!("Failed to set properties for object '{}'", object_id))
        })?;
        Arc::get_mut(&mut self.kb).unwrap().set_properties(object_id, values).unwrap_or_else(|e| {
            eprintln!("Error setting properties for object '{}' in knowledge base: {:?}", object_id, e);
        });
        Ok(())
    }

    pub async fn add_data(&mut self, object_id: &str, values: HashMap<String, Value>, date_time: DateTime<Utc>) -> Result<(), CoCoError> {
        self.db.add_data(object_id, &values, &date_time).await.map_err(|e| {
            eprintln!("Error adding data to object '{}' in database: {:?}", object_id, e);
            CoCoError::ConnectionError(format!("Failed to add data to object '{}'", object_id))
        })?;
        Arc::get_mut(&mut self.kb).unwrap().add_values(object_id, values, date_time).unwrap_or_else(|e| {
            eprintln!("Error adding data to object '{}' in knowledge base: {:?}", object_id, e);
        });
        Ok(())
    }

    pub fn get_rules(&self) -> Vec<Rule> {
        self.kb.get_rules()
    }

    pub fn get_rule(&self, name: &str) -> Option<Rule> {
        self.kb.get_rule(name)
    }

    pub async fn create_new_rule(&mut self, name: &str, content: &str) -> Result<(), CoCoError> {
        self.create_rule(Rule { name: name.to_string(), content: content.to_string() }).await
    }

    pub async fn create_rule(&mut self, rule: Rule) -> Result<(), CoCoError> {
        self.db.create_rule(&rule).await.map_err(|e| {
            eprintln!("Error creating rule '{}' in database: {:?}", rule.name, e);
            CoCoError::ConnectionError(format!("Failed to create rule '{}'", rule.name))
        })?;
        self.add_rules(vec![rule]);
        Ok(())
    }

    fn add_classes(&mut self, classes: Vec<Class>) {
        for class in classes {
            let class_name = class.name.clone();
            let kb = Arc::get_mut(&mut self.kb).unwrap();
            kb.create_class(class).unwrap_or_else(|e| {
                eprintln!("Error adding class '{}' to knowledge base: {:?}", class_name, e);
            });
        }
    }

    fn add_objects(&mut self, objects: Vec<Object>) {
        for object in objects {
            let object_id = object.id.clone().unwrap();
            let kb = Arc::get_mut(&mut self.kb).unwrap();
            kb.create_object(object).unwrap_or_else(|e| {
                eprintln!("Error adding object '{}' to knowledge base: {:?}", object_id, e);
            });
        }
    }

    fn add_rules(&mut self, rules: Vec<Rule>) {
        for rule in rules {
            let rule_name = rule.name.clone();
            let kb = Arc::get_mut(&mut self.kb).unwrap();
            kb.create_rule(rule).unwrap_or_else(|e| {
                eprintln!("Error adding rule '{}' to knowledge base: {:?}", rule_name, e);
            });
        }
    }
}

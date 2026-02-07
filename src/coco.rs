use crate::{Class, CoCoEvent, DataStore, KnowledgeBase, Object, Property, Rule, Value};
use chrono::{DateTime, Utc};
use std::{
    collections::{HashMap, HashSet},
    error::Error,
    sync::{Arc, Mutex},
};
use tokio::sync::broadcast;

pub struct CoCo {
    sender: broadcast::Sender<CoCoEvent>,
    db: Arc<dyn DataStore>,
    kb: Arc<Mutex<dyn KnowledgeBase>>,
}

impl CoCo {
    pub async fn new(db: Arc<dyn DataStore>, kb: Arc<Mutex<dyn KnowledgeBase>>) -> Self {
        let sender = kb.lock().unwrap().get_event_sender();
        let coco = Self { sender, db: db.clone(), kb: kb.clone() };
        let mut receiver = coco.sender.subscribe();
        tokio::spawn(async move {
            while let Ok(event) = receiver.recv().await {
                if let CoCoEvent::AddedValues(object_id, values) = event {
                    println!("Added values to object {}: {:?}", object_id, values);
                }
            }
        });

        coco.add_classes(db.get_classes().await.expect("Failed to load classes from database")).expect("Failed to add classes to knowledge base");
        coco.add_objects(db.get_objects().await.expect("Failed to load objects from database")).expect("Failed to add objects to knowledge base");
        coco.add_rules(db.get_rules().await.expect("Failed to load rules from database")).expect("Failed to add rules to knowledge base");

        coco
    }

    pub fn get_event_sender(&self) -> broadcast::Sender<CoCoEvent> {
        self.sender.clone()
    }

    pub fn get_classes(&self) -> Vec<Class> {
        self.kb.lock().unwrap().get_classes()
    }

    pub fn get_class(&self, name: &str) -> Option<Class> {
        self.kb.lock().unwrap().get_class(name)
    }

    pub async fn create_new_class(&self, name: &str, parents: Option<HashSet<String>>, static_properties: Option<HashMap<String, Property>>, dynamic_properties: Option<HashMap<String, Property>>) -> Result<(), Box<dyn Error>> {
        self.create_class(Class { name: name.to_string(), parents, static_properties, dynamic_properties }).await
    }

    pub async fn create_class(&self, class: Class) -> Result<(), Box<dyn Error>> {
        self.db.create_class(&class).await.expect("Failed to create class in database");
        self.add_classes(vec![class])?;
        Ok(())
    }

    fn add_classes(&self, classes: Vec<Class>) -> Result<(), Box<dyn Error>> {
        for class in classes {
            self.kb.lock().unwrap().create_class(&class)?;
        }
        Ok(())
    }

    pub fn get_objects(&self) -> Vec<Object> {
        self.kb.lock().unwrap().get_objects()
    }

    pub fn get_object(&self, id: &str) -> Option<Object> {
        self.kb.lock().unwrap().get_object(id)
    }

    pub async fn create_new_object(&self, classes: HashSet<String>, properties: Option<HashMap<String, Value>>, values: Option<HashMap<String, (Value, DateTime<Utc>)>>) -> Result<(), Box<dyn Error>> {
        self.create_object(Object { id: None, classes, properties, values }).await
    }

    pub async fn create_object(&self, object: Object) -> Result<(), Box<dyn Error>> {
        let id = self.db.create_object(&object).await?;
        let object = Object { id: Some(id), ..object };
        self.add_objects(vec![object])?;
        Ok(())
    }

    pub async fn set_properties(&self, object: &Object, values: &HashMap<String, Value>) -> Result<(), Box<dyn Error>> {
        self.db.set_properties(object, values).await?;
        self.kb.lock().unwrap().set_properties(object, values)?;
        Ok(())
    }

    pub async fn add_data(&self, object: &Object, values: &HashMap<String, Value>, date_time: &DateTime<Utc>) -> Result<(), Box<dyn Error>> {
        self.db.set_values(object, values, date_time).await?;
        self.kb.lock().unwrap().add_data(object, values, date_time)?;
        Ok(())
    }

    fn add_objects(&self, objects: Vec<Object>) -> Result<(), Box<dyn Error>> {
        for object in objects {
            self.kb.lock().unwrap().create_object(&object).expect("Failed to create object in knowledge base");
        }
        Ok(())
    }

    pub async fn set_values(&self, object: &mut Object, values: &HashMap<String, Value>) -> Result<(), Box<dyn Error>> {
        let date_time: DateTime<Utc> = Utc::now();
        self.db.set_values(object, values, &date_time).await?;
        self.kb.lock().unwrap().add_data(object, values, &date_time)?;
        Ok(())
    }

    pub fn get_rules(&self) -> Vec<Rule> {
        self.kb.lock().unwrap().get_rules()
    }

    pub fn get_rule(&self, name: &str) -> Option<Rule> {
        self.kb.lock().unwrap().get_rule(name)
    }

    pub async fn create_new_rule(&self, name: &str, content: &str) -> Result<(), Box<dyn Error>> {
        self.create_rule(Rule { name: name.to_string(), content: content.to_string() }).await
    }

    pub async fn create_rule(&self, rule: Rule) -> Result<(), Box<dyn Error>> {
        self.db.create_rule(&rule).await?;
        self.add_rules(vec![rule])?;
        Ok(())
    }

    fn add_rules(&self, db_rules: Vec<Rule>) -> Result<(), Box<dyn Error>> {
        for rule in db_rules {
            self.kb.lock().unwrap().create_rule(&rule)?;
        }
        Ok(())
    }

    pub async fn drop_db(&self) -> Result<(), Box<dyn Error>> {
        self.db.drop_db().await?;
        Ok(())
    }
}

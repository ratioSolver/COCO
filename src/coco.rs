use crate::{Class, CoCoEvent, DataStore, KnowledgeBase, Object, Property, Rule, Value};
use chrono::{DateTime, Utc};
use std::{
    collections::{HashMap, HashSet},
    error::Error,
    sync::{Arc, Mutex, RwLock},
};
use tokio::sync::broadcast;

pub struct CoCo {
    sender: broadcast::Sender<CoCoEvent>,
    db: Arc<dyn DataStore>,
    kb: Arc<Mutex<dyn KnowledgeBase>>,
    classes: RwLock<HashMap<String, Class>>,
    objects: RwLock<HashMap<String, Object>>,
    rules: RwLock<HashMap<String, Rule>>,
}

impl CoCo {
    pub async fn new(db: Arc<dyn DataStore>, kb: Arc<Mutex<dyn KnowledgeBase>>) -> Self {
        let sender = kb.lock().unwrap().get_event_sender();
        let coco = Self {
            sender,
            db: db.clone(),
            kb: kb.clone(),
            classes: RwLock::new(HashMap::new()),
            objects: RwLock::new(HashMap::new()),
            rules: RwLock::new(HashMap::new()),
        };
        let mut receiver = coco.sender.subscribe();
        tokio::spawn(async move {
            while let Ok(event) = receiver.recv().await {
                if let CoCoEvent::AddedValues(object_id, values) = event {
                    println!("Added values to object {}: {:?}", object_id, values);
                }
            }
        });
        coco
    }

    pub fn get_event_sender(&self) -> broadcast::Sender<CoCoEvent> {
        self.sender.clone()
    }

    pub fn get_classes(&self) -> Vec<Class> {
        self.classes.read().unwrap().values().cloned().collect()
    }

    pub fn get_class(&self, name: &str) -> Option<Class> {
        self.classes.read().unwrap().get(name).cloned()
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
            self.classes.write().unwrap().insert(class.name.clone(), class.clone());
            self.sender.send(CoCoEvent::ClassCreated(class)).expect("Failed to send ClassCreated event");
        }
        Ok(())
    }

    pub fn get_objects(&self) -> Vec<Object> {
        self.objects.read().unwrap().values().cloned().collect()
    }

    pub fn get_object(&self, id: &str) -> Option<Object> {
        self.objects.read().unwrap().get(id).cloned()
    }

    pub async fn create_new_object(&self, classes: HashSet<String>, properties: Option<HashMap<String, Value>>, values: Option<HashMap<String, (Value, DateTime<Utc>)>>) -> Result<(), Box<dyn Error>> {
        self.create_object(Object { id: String::new(), classes, properties, values }).await
    }

    pub async fn create_object(&self, object: Object) -> Result<(), Box<dyn Error>> {
        let id = self.db.create_object(&object).await?;
        let object = Object { id, ..object };
        self.add_objects(vec![object])?;
        Ok(())
    }

    fn add_objects(&self, objects: Vec<Object>) -> Result<(), Box<dyn Error>> {
        let class_guard = self.classes.read().unwrap();
        for object in objects {
            for class_name in &object.classes {
                let class = class_guard.get(class_name).expect("Class not found for object");
                self.kb.lock().unwrap().create_object(class, &object).expect("Failed to create object in knowledge base");
            }
            self.objects.write().expect("Failed to lock objects for writing").insert(object.id.clone(), object.clone());
            self.sender.send(CoCoEvent::ObjectCreated(object)).expect("Failed to send ObjectCreated event");
        }
        Ok(())
    }

    pub async fn set_values(&self, object: &mut Object, values: &HashMap<String, Value>) -> Result<(), Box<dyn Error>> {
        let date_time: DateTime<Utc> = Utc::now();
        self.db.set_values(object, values, &date_time).await?;
        let class_guard = self.classes.read().unwrap();
        for class_name in &object.classes {
            let class = class_guard.get(class_name).expect("Class not found for object");
            self.kb.lock().unwrap().add_data(class, object, values, &date_time)?;
        }
        Ok(())
    }

    pub fn get_rules(&self) -> Vec<Rule> {
        self.rules.read().unwrap().values().cloned().collect()
    }

    pub fn get_rule(&self, name: &str) -> Option<Rule> {
        self.rules.read().unwrap().get(name).cloned()
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
            self.rules.write().unwrap().insert(rule.name.clone(), rule);
        }
        Ok(())
    }

    pub async fn drop_db(&self) -> Result<(), Box<dyn Error>> {
        self.db.drop_db().await?;
        Ok(())
    }
}

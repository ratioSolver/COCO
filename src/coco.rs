use crate::{Class, CoCoEvent, DataStore, KnowledgeBase, Object, Property, Rule};
use std::{
    collections::{HashMap, HashSet},
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
                match event {
                    CoCoEvent::AddedValues(object_id, values) => {
                        println!("Added values to object {}: {:?}", object_id, values);
                    }
                    _ => {}
                }
            }
        });
        coco
    }

    pub fn get_event_sender(&self) -> broadcast::Sender<CoCoEvent> {
        self.sender.clone()
    }

    pub async fn create_new_class(&self, name: &str, parents: Option<HashSet<String>>, static_properties: Option<HashMap<String, Property>>, dynamic_properties: Option<HashMap<String, Property>>) {
        let class = Class { name: name.to_string(), parents, static_properties, dynamic_properties };
        self.db.create_class(&class).await.expect("Failed to create class in database");
        self.add_classes(vec![class]);
    }

    pub async fn create_class(&self, class: &Class) {
        self.db.create_class(class).await.expect("Failed to create class in database");
        self.add_classes(vec![class.clone()]);
    }

    fn add_classes(&self, classes: Vec<Class>) {
        for class in &classes {
            self.kb.lock().unwrap().create_class(class);
            self.classes.write().unwrap().insert(class.name.clone(), class.clone());
            self.sender.send(CoCoEvent::ClassCreated(class.clone())).expect("Failed to send ClassCreated event");
        }
    }
}

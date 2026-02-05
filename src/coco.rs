use crate::{DataStore, KnowledgeBase};
use std::sync::{Arc, Mutex};

#[derive(Clone)]
pub enum CoCoEvent {
    ClassCreated(String),
    ObjectCreated(String),
    AddedValues(String, String, Vec<String>),
}

pub struct CoCo {
    db: Arc<dyn DataStore>,
    kb: Arc<Mutex<dyn KnowledgeBase>>,
}

impl CoCo {
    pub async fn new(db: Arc<dyn DataStore>, kb: Arc<Mutex<dyn KnowledgeBase>>) -> Self {
        let coco = Self { db: db.clone(), kb: kb.clone() };
        let mut sender = kb.lock().unwrap().get_event_sender().subscribe();
        tokio::spawn(async move {
            while let Ok(event) = sender.recv().await {
                match event {
                    CoCoEvent::ClassCreated(class_name) => {
                        println!("Class created: {}", class_name);
                    }
                    CoCoEvent::ObjectCreated(object_id) => {
                        println!("Object created: {}", object_id);
                    }
                    CoCoEvent::AddedValues(object_id, attribute, values) => {
                        println!("Added values to {}: {} -> {:?}", object_id, attribute, values);
                        db.add_class(object_id.as_str()).await.unwrap();
                    }
                }
            }
        });
        coco
    }

    pub async fn add_class(&self, class_name: &str) -> Result<(), String> {
        self.db.add_class(class_name).await?;
        self.kb.lock().unwrap().add_class(class_name);
        Ok(())
    }
}

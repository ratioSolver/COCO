use crate::{
    db::Database,
    kb::KnowledgeBase,
    model::{Class, CoCoEvent},
};
use std::sync::Arc;

pub mod db;
pub mod kb;
pub mod model;

pub struct CoCo {
    db: Arc<dyn Database>,
    kb: Arc<dyn KnowledgeBase>,
}

impl CoCo {
    pub async fn new(db: Arc<dyn Database>, kb: Arc<dyn KnowledgeBase>) -> Self {
        let mut coco = CoCo { db: db.clone(), kb: kb.clone() };
        coco.add_classes(db.get_classes().await.unwrap_or_else(|e| {
            eprintln!("Error fetching classes from database: {:?}", e);
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

    fn add_classes(&mut self, classes: Vec<Class>) {
        for class in classes {
            let class_name = class.name.clone();
            let kb = Arc::get_mut(&mut self.kb).unwrap();
            kb.create_class(class).unwrap_or_else(|e| {
                eprintln!("Error adding class '{}' to knowledge base: {:?}", class_name, e);
            });
        }
    }
}

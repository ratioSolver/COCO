use crate::{
    db::{Database, setup_db},
    kb::{KnowledgeBase, setup_kb},
    model::CoCoEvent,
};
use std::sync::{Arc, Mutex};
use tracing::error;

pub mod db;
pub mod kb;
pub mod model;

pub type Callback = Box<dyn Fn(CoCoEvent) + Send + Sync + 'static>;

pub struct CoCo<KB: KnowledgeBase> {
    kb: KB,
    callback: Arc<Mutex<Option<Callback>>>,
}

impl<KB: KnowledgeBase> CoCo<KB> {
    pub fn new(kb: KB, db: Arc<dyn Database + 'static>) -> Self {
        let callback = Arc::new(Mutex::new(None));
        let coco = Self { kb, callback: callback.clone() };

        coco.kb.set_callback(move |query| match query {
            kb::KnowledgeBaseEvent::AddedClass(object_id, class) => {
                let db = db.clone();
                let callback = callback.clone();
                tokio::spawn(async move {
                    match db.add_class(&object_id, &class).await {
                        Ok(_) => {
                            if let Some(cb) = callback.lock().unwrap().as_ref() {
                                cb(CoCoEvent::AddedClass(object_id.clone(), class.clone()));
                            }
                        }
                        Err(e) => error!("Failed to add class to database: {}", e),
                    }
                });
            }
            kb::KnowledgeBaseEvent::UpdatedProperties(object_id, properties) => {
                let db = db.clone();
                let callback = callback.clone();
                tokio::spawn(async move {
                    match db.set_properties(&object_id, &properties).await {
                        Ok(_) => {
                            if let Some(cb) = callback.lock().unwrap().as_ref() {
                                cb(CoCoEvent::UpdatedProperties(object_id.clone(), properties.clone()));
                            }
                        }
                        Err(e) => error!("Failed to update properties in database: {}", e),
                    }
                });
            }
            kb::KnowledgeBaseEvent::AddedValues(object_id, values, date_time) => {
                let db = db.clone();
                let callback = callback.clone();
                tokio::spawn(async move {
                    match db.add_data(&object_id, &values, &date_time).await {
                        Ok(_) => {
                            if let Some(cb) = callback.lock().unwrap().as_ref() {
                                cb(CoCoEvent::AddedValues(object_id.clone(), values.clone(), date_time));
                            }
                        }
                        Err(e) => error!("Failed to add values to database: {}", e),
                    }
                });
            }
            kb::KnowledgeBaseEvent::LLMPrompt(_object_id, _message) => {}
            kb::KnowledgeBaseEvent::Message(_object_id, _title, _message) => {}
        });
        coco
    }

    pub async fn default() -> Result<CoCo<impl KnowledgeBase>, Box<dyn std::error::Error>> {
        let kb = setup_kb().map_err(|e| format!("Failed to set up knowledge base: {}", e))?;
        let db = setup_db().await.map_err(|e| format!("Failed to set up database: {}", e))?;
        Ok(CoCo::new(kb, Arc::new(db)))
    }

    pub fn set_callback(&mut self, callback: Callback) {
        self.callback.lock().unwrap().replace(callback);
    }
}

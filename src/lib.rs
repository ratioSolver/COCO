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
            _ => {}
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

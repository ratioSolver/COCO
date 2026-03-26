use crate::{
    db::{Database, setup_db},
    kb::{KnowledgeBase, setup_kb},
};
use std::sync::Arc;
use tracing::error;

pub mod db;
pub mod kb;
pub mod model;

pub struct CoCo<KB: KnowledgeBase, DB: Database + 'static> {
    kb: KB,
    db: Arc<DB>,
}

impl<KB: KnowledgeBase, DB: Database> CoCo<KB, DB> {
    pub fn new(kb: KB, db: Arc<DB>) -> Self {
        let coco = Self { kb, db: db.clone() };
        coco.kb.set_callback(move |query| match query {
            kb::KnowledgeBaseEvent::AddedClass(object_id, class) => {
                let db = db.clone();
                tokio::spawn(async move {
                    if let Err(e) = db.add_class(&object_id, &class).await {
                        error!("Failed to add class '{}' to object '{}': {}", class, object_id, e);
                    }
                });
            }
            _ => {}
        });
        coco
    }

    pub async fn default() -> Result<CoCo<impl KnowledgeBase, impl Database + 'static>, Box<dyn std::error::Error>> {
        let kb = setup_kb().map_err(|e| format!("Failed to set up knowledge base: {}", e))?;
        let db = setup_db().await.map_err(|e| format!("Failed to set up database: {}", e))?;
        Ok(CoCo::new(kb, Arc::new(db)))
    }
}

use crate::{db::Database, kb::KnowledgeBase};
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
}

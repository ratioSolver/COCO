use crate::{
    db::{Database, DatabaseError, setup_db},
    kb::{KnowledgeBase, KnowledgeBaseError, setup_kb},
    model::{CoCoError, CoCoEvent},
};
use std::{
    fs,
    path::Path,
    sync::{Arc, Mutex},
};
use tracing::{error, info, trace};

pub mod db;
pub mod kb;
pub mod model;
#[cfg(feature = "server")]
pub mod server;

pub type Callback = Box<dyn Fn(CoCoEvent) + Send + Sync + 'static>;

pub struct CoCo<KB: KnowledgeBase, DB: Database + 'static> {
    kb: KB,
    db: Arc<DB>,
    callback: Arc<Mutex<Option<Callback>>>,
}

impl<KB: KnowledgeBase, DB: Database + 'static> CoCo<KB, DB> {
    pub fn new(kb: KB, db: Arc<DB>) -> Self {
        let callback = Arc::new(Mutex::new(None));
        let coco = Self { kb, db: db.clone(), callback: callback.clone() };

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
        });
        coco
    }

    pub async fn default() -> Result<CoCo<impl KnowledgeBase, impl Database>, Box<dyn std::error::Error>> {
        let kb = setup_kb().map_err(|e| format!("Failed to set up knowledge base: {}", e))?;
        let db = setup_db().await.map_err(|e| format!("Failed to set up database: {}", e))?;
        Ok(CoCo::new(kb, Arc::new(db)))
    }

    pub async fn load_classes<P: AsRef<Path>>(&mut self, path: P) -> Result<(), CoCoError> {
        info!("Loading classes from directory '{}'", path.as_ref().display());
        let entries = fs::read_dir(path).map_err(|e| CoCoError::FileReadError(e.to_string()))?;
        for entry in entries {
            let entry = entry.map_err(|e| CoCoError::FileReadError(e.to_string()))?;
            let path = entry.path();
            if path.is_file() {
                let content = fs::read_to_string(&path).map_err(|e| CoCoError::FileReadError(e.to_string()))?;
                let class: model::Class = serde_json::from_str(&content).map_err(|e| CoCoError::JsonParseError(e.to_string()))?;
                if self.kb.get_class(&class.name).is_none() {
                    info!("Creating class '{}' with parents {:?}, static properties {:?}, and dynamic properties {:?}", class.name, class.parents, class.static_properties, class.dynamic_properties);
                    self.db.create_class(&class).await.map_err(map_db_error)?;
                    self.kb.create_class(class).map_err(map_kb_error)?;
                } else {
                    trace!("Class '{}' already exists, skipping '{}'", class.name, path.display());
                }
            }
        }
        Ok(())
    }

    pub fn set_callback(&mut self, callback: Callback) {
        self.callback.lock().unwrap().replace(callback);
    }
}

fn map_db_error(error: DatabaseError) -> CoCoError {
    match error {
        DatabaseError::ClassAlreadyExists(name) => CoCoError::ClassAlreadyExists(name),
        DatabaseError::ClassNotFound(name) => CoCoError::ClassNotFound(name),
        DatabaseError::ObjectAlreadyExists(id) => CoCoError::ObjectAlreadyExists(id),
        DatabaseError::ObjectNotFound(id) => CoCoError::ObjectNotFound(id),
        DatabaseError::RuleAlreadyExists(name) => CoCoError::RuleAlreadyExists(name),
        DatabaseError::RuleNotFound(name) => CoCoError::RuleNotFound(name),
        other => CoCoError::DatabaseError(format!("Database error: {:?}", other)),
    }
}

fn map_kb_error(error: KnowledgeBaseError) -> CoCoError {
    match error {
        KnowledgeBaseError::ClassAlreadyExists(name) => CoCoError::ClassAlreadyExists(name),
        KnowledgeBaseError::ClassNotFound(name) => CoCoError::ClassNotFound(name),
        KnowledgeBaseError::ObjectAlreadyExists(id) => CoCoError::ObjectAlreadyExists(id),
        KnowledgeBaseError::ObjectNotFound(id) => CoCoError::ObjectNotFound(id),
        KnowledgeBaseError::RuleAlreadyExists(name) => CoCoError::RuleAlreadyExists(name),
        KnowledgeBaseError::RuleNotFound(name) => CoCoError::RuleNotFound(name),
        other => CoCoError::KnowledgeBaseError(format!("Knowledge base error: {:?}", other)),
    }
}

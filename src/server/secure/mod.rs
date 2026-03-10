use async_trait::async_trait;
use std::sync::Arc;

use crate::{CoCo, server::secure::secure::CoCoState};

pub mod secure;

#[cfg(feature = "mongodb")]
pub mod mongodb;

#[derive(Debug)]
pub enum DatabaseError {
    ConnectionError(String),
}

#[async_trait]
pub trait Database: Send + Sync {
    fn secret(&self) -> &str;

    async fn login(&self, username: &str, hashed_password: &str) -> Option<String>;
    async fn register(&self, username: &str, hashed_password: &str, role: &str) -> bool;
}

#[derive(Clone)]
pub struct SecureCoCoState {
    coco: Arc<CoCo>,
    db: Arc<dyn Database>,
}

impl SecureCoCoState {
    pub fn new(coco: Arc<CoCo>, db: Arc<dyn Database>) -> Self {
        Self { coco, db }
    }

    pub async fn default() -> Self {
        Self { coco: Arc::new(CoCo::default().await), db: setup_db().await }
    }
}

impl CoCoState for SecureCoCoState {
    fn coco(&self) -> Arc<CoCo> {
        self.coco.clone()
    }

    fn users_db(&self) -> Arc<dyn Database> {
        self.db.clone()
    }
}

pub async fn setup_db() -> Arc<dyn Database> {
    #[cfg(feature = "mongodb")]
    return setup_mongodb().await;

    #[cfg(not(feature = "mongodb"))]
    panic!("No database backend configured");
}

#[cfg(feature = "mongodb")]
async fn setup_mongodb() -> Arc<dyn Database> {
    use crate::server::secure::mongodb::MongoDB;

    let name = std::env::var("USERS_DB_NAME").unwrap_or_else(|_| "coco_users".to_owned());
    let host = std::env::var("USERS_DB_HOST").unwrap_or_else(|_| "localhost".to_owned());
    let port = std::env::var("USERS_DB_PORT").ok().and_then(|p| p.parse().ok()).unwrap_or(27017);
    let uri = format!("mongodb://{}:{}", host, port);
    Arc::new(MongoDB::new(&name, &uri).await.unwrap())
}

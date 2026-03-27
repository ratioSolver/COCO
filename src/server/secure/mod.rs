use crate::{CoCo, kb::KnowledgeBase, model::CoCoEvent, server::secure::secure_server::CoCoState};
use argon2::{
    Argon2, PasswordHash, PasswordHasher, PasswordVerifier,
    password_hash::{SaltString, rand_core::OsRng},
};
use async_trait::async_trait;
use chrono::{Duration, Utc};
use jsonwebtoken::{DecodingKey, EncodingKey, Header, Validation, errors::Error};
use serde::{Deserialize, Serialize};
use std::sync::Arc;
use tokio::sync::{RwLock, broadcast};
use tracing::trace;
use utoipa::ToSchema;

pub mod secure_server;

#[cfg(feature = "mongodb")]
pub mod mongodb;

#[derive(Debug)]
pub enum DatabaseError {
    ConnectionError(String),
    UserAlreadyExists(String),
    UserNotFound(String),
    Unauthorized(String),
}

#[derive(Clone, Serialize, Deserialize, Debug, ToSchema)]
pub struct User {
    pub username: String,
    pub role: String,
}

#[async_trait]
pub trait Database: Send + Sync {
    fn secret(&self) -> &str;

    async fn get_users(&self) -> Result<Vec<User>, DatabaseError>;
    async fn get_user(&self, username: &str, password: &str) -> Result<User, DatabaseError>;
    async fn create_user(&self, username: &str, password: &str, role: &str) -> Result<(), DatabaseError>;
}

pub struct SecureCoCoState<KB: KnowledgeBase> {
    coco: Arc<RwLock<CoCo<KB>>>,
    event_tx: broadcast::Sender<CoCoEvent>,
    db: Arc<dyn Database>,
}

impl<KB: KnowledgeBase> Clone for SecureCoCoState<KB> {
    fn clone(&self) -> Self {
        Self { coco: self.coco.clone(), event_tx: self.event_tx.clone(), db: self.db.clone() }
    }
}

impl<KB: KnowledgeBase> SecureCoCoState<KB> {
    pub async fn new(coco: Arc<RwLock<CoCo<KB>>>, db: Arc<dyn Database>) -> Self {
        let (event_tx, _) = broadcast::channel(100);
        coco.write().await.set_callback(Arc::new({
            let event_tx = event_tx.clone();
            move |event| {
                trace!("CoCo event occurred: {:?}", event);
                let _ = event_tx.send(event);
            }
        }));
        Self { coco, event_tx, db }
    }
}

impl<KB: KnowledgeBase> CoCoState<KB> for SecureCoCoState<KB> {
    fn coco(&self) -> Arc<RwLock<CoCo<KB>>> {
        self.coco.clone()
    }

    fn event_tx(&self) -> broadcast::Sender<CoCoEvent> {
        self.event_tx.clone()
    }

    fn users_db(&self) -> Arc<dyn Database> {
        self.db.clone()
    }
}

pub fn hash_password(password: &str) -> String {
    let salt = SaltString::generate(&mut OsRng);
    Argon2::default().hash_password(password.as_bytes(), &salt).unwrap().to_string()
}

pub fn verify_password(password: &str, hash: &str) -> bool {
    let parsed_hash = PasswordHash::new(hash).unwrap();
    Argon2::default().verify_password(password.as_bytes(), &parsed_hash).is_ok()
}

#[derive(Debug, Serialize, Deserialize)]
pub struct Claims {
    sub: String,
    exp: usize,
    role: String,
    #[serde(default = "default_token_type")]
    token_type: String,
}

fn default_token_type() -> String {
    "access".to_owned()
}

pub fn create_jwt(user_id: &str, role: &str, secret: &str) -> Result<String, Error> {
    let now = Utc::now();
    let expire = now + Duration::hours(24);

    let claims = Claims {
        sub: user_id.to_owned(),
        exp: expire.timestamp() as usize,
        role: role.to_owned(),
        token_type: "access".to_owned(),
    };

    jsonwebtoken::encode(&Header::default(), &claims, &EncodingKey::from_secret(secret.as_ref()))
}

pub fn create_refresh_jwt(user_id: &str, role: &str, secret: &str) -> Result<String, Error> {
    let now = Utc::now();
    let expire = now + Duration::days(30);

    let claims = Claims {
        sub: user_id.to_owned(),
        exp: expire.timestamp() as usize,
        role: role.to_owned(),
        token_type: "refresh".to_owned(),
    };

    jsonwebtoken::encode(&Header::default(), &claims, &EncodingKey::from_secret(secret.as_ref()))
}

pub fn verify_jwt(token: &str, secret: &str) -> Result<Claims, Error> {
    let decoding_key = DecodingKey::from_secret(secret.as_ref());
    let validation = Validation::default();
    let token_data = jsonwebtoken::decode::<Claims>(token, &decoding_key, &validation)?;
    Ok(token_data.claims)
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

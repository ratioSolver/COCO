use crate::{
    CoCo,
    server::{CoCoState, auth::SecureCoCoState},
};
use async_trait::async_trait;
use mongodb::Client;
use std::sync::Arc;

#[derive(Clone)]
pub struct MongoSecureCoCoState {
    coco: Arc<CoCo>,
    name: String,
    client: Client,
    secret: String,
}

impl MongoSecureCoCoState {
    pub async fn new(coco: Arc<CoCo>, name: &str, uri: &str) -> Result<Self, String> {
        let client = Client::with_uri_str(uri).await.map_err(|e| format!("Failed to connect to MongoDB: {}", e))?;
        Ok(Self {
            coco,
            name: name.to_owned(),
            client,
            secret: std::env::var("JWT_SECRET").unwrap_or_else(|_| "default_secret".to_string()),
        })
    }
}

impl CoCoState for MongoSecureCoCoState {
    fn coco(&self) -> Arc<CoCo> {
        self.coco.clone()
    }
}

#[async_trait]
impl SecureCoCoState for MongoSecureCoCoState {
    fn secret(&self) -> &str {
        &self.secret
    }

    async fn login(&self, username: &str, hashed_password: &str) -> Option<String> {
        // Implement MongoDB logic to verify user credentials and return role
        None
    }

    async fn register(&self, username: &str, hashed_password: &str, role: &str) -> bool {
        // Implement MongoDB logic to create a new user with the given credentials and role
        true
    }
}

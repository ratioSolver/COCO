use crate::server::secure::{Database, DatabaseError};
use async_trait::async_trait;
use mongodb::Client;

#[derive(Clone)]
pub struct MongoDB {
    secret: String,
    name: String,
    client: Client,
}

impl MongoDB {
    pub async fn new(name: &str, connection_string: &str) -> Result<Self, DatabaseError> {
        let client = Client::with_uri_str(connection_string).await.map_err(|e| DatabaseError::ConnectionError(e.to_string()))?;
        Ok(Self {
            secret: std::env::var("JWT_SECRET").unwrap_or_else(|_| "default_secret".to_owned()),
            name: name.to_owned(),
            client,
        })
    }
}

#[async_trait]
impl Database for MongoDB {
    fn secret(&self) -> &str {
        &self.secret
    }

    async fn login(&self, username: &str, hashed_password: &str) -> Option<String> {
        // Implement MongoDB-based login logic here
        None
    }

    async fn register(&self, username: &str, hashed_password: &str, role: &str) -> bool {
        // Implement MongoDB-based registration logic here
        false
    }
}

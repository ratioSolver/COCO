use crate::server::secure::{Database, DatabaseError};
use async_trait::async_trait;
use mongodb::bson::doc;
use mongodb::{Client, IndexModel, bson::Document, options::IndexOptions};

#[derive(Clone)]
pub struct MongoDB {
    secret: String,
    name: String,
    client: Client,
}

impl MongoDB {
    pub async fn new(name: &str, connection_string: &str) -> Result<Self, DatabaseError> {
        let client = Client::with_uri_str(connection_string).await.map_err(|e| DatabaseError::ConnectionError(e.to_string()))?;
        let db = client.database(name);
        let collection_names = db.list_collection_names().await.map_err(|e| DatabaseError::ConnectionError(e.to_string()))?;
        if collection_names.is_empty() {
            let users_collection = db.collection::<Document>("users");
            let index = IndexModel::builder().keys(doc! { "username": 1 }).options(IndexOptions::builder().unique(true).build()).build();
            users_collection.create_index(index).await.map_err(|e| DatabaseError::ConnectionError(e.to_string()))?;
        }
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
        let db = self.client.database(&self.name);
        let users_collection = db.collection::<Document>("users");
        if let Ok(Some(user_doc)) = users_collection.find_one(doc! { "username": username }).await
            && let (Some(stored_hash), Some(role)) = (user_doc.get_str("password").ok(), user_doc.get_str("role").ok())
            && stored_hash == hashed_password
        {
            return Some(role.to_owned());
        }
        None
    }

    async fn register(&self, username: &str, hashed_password: &str, role: &str) -> bool {
        let db = self.client.database(&self.name);
        let users_collection = db.collection::<Document>("users");
        let new_user = doc! { "username": username, "password": hashed_password, "role": role };
        users_collection.insert_one(new_user).await.is_ok()
    }
}

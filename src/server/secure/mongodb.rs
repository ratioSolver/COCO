use crate::server::secure::{Database, DatabaseError, User, hash_password, verify_password};
use async_trait::async_trait;
use futures::TryStreamExt;
use mongodb::bson::doc;
use mongodb::{Client, IndexModel, bson::Document, options::IndexOptions};
use serde::{Deserialize, Serialize};

#[derive(Clone)]
pub struct MongoDB {
    secret: String,
    name: String,
    client: Client,
}

#[derive(Serialize, Deserialize, Debug)]
struct MongoUser {
    username: String,
    password: String,
    role: String,
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

    async fn get_users(&self) -> Result<Vec<User>, DatabaseError> {
        let db = self.client.database(&self.name);
        let collection = db.collection::<MongoUser>("users");
        let cursor = collection.find(doc! {}).await.map_err(|e| DatabaseError::ConnectionError(e.to_string()))?;
        let users: Vec<MongoUser> = cursor.try_collect().await.map_err(|e| DatabaseError::ConnectionError(e.to_string()))?;
        Ok(users.into_iter().map(|u| User { username: u.username, role: u.role }).collect())
    }

    async fn get_user(&self, username: &str, password: &str) -> Result<User, DatabaseError> {
        let db = self.client.database(&self.name);
        let users_collection = db.collection::<MongoUser>("users");
        let filter = doc! { "username": username };
        let user = users_collection.find_one(filter).await.map_err(|e| DatabaseError::UserNotFound(e.to_string()))?;
        match user {
            Some(user) if verify_password(password, &user.password) => Ok(User { username: user.username, role: user.role }),
            _ => Err(DatabaseError::Unauthorized("Invalid username or password".to_string())),
        }
    }

    async fn create_user(&self, username: &str, password: &str, role: &str) -> Result<(), DatabaseError> {
        let db = self.client.database(&self.name);
        let collection = db.collection::<MongoUser>("users");
        collection
            .insert_one(MongoUser { username: username.to_owned(), password: hash_password(password), role: role.to_owned() })
            .await
            .map_err(|e| if e.to_string().contains("duplicate key error") { DatabaseError::UserAlreadyExists(e.to_string()) } else { DatabaseError::ConnectionError(e.to_string()) })?;
        Ok(())
    }
}

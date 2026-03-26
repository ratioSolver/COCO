use crate::db::{Database, DatabaseError};
use crate::model::{Class, TimedValue, Value};
use async_trait::async_trait;
use mongodb::bson::oid::ObjectId;
use mongodb::bson::{self, doc};
use mongodb::{Client, IndexModel, bson::Document, options::IndexOptions};
use serde::{Deserialize, Serialize};
use std::collections::{HashMap, HashSet};

#[derive(Serialize, Deserialize, Debug)]
struct MongoObject {
    #[serde(rename = "_id", skip_serializing_if = "Option::is_none")]
    pub id: Option<ObjectId>,
    pub classes: HashSet<String>,
    #[serde(skip_serializing_if = "Option::is_none")]
    pub properties: Option<HashMap<String, Value>>,
    #[serde(skip_serializing_if = "Option::is_none")]
    pub values: Option<HashMap<String, TimedValue>>,
}

pub struct MongoDB {
    name: String,
    client: Client,
}

impl MongoDB {
    pub async fn new(name: &str, connection_string: &str) -> Result<Self, DatabaseError> {
        let client = Client::with_uri_str(connection_string).await.map_err(|e| DatabaseError::ConnectionError(e.to_string()))?;
        let db = client.database(name);
        let collection_names = db.list_collection_names().await.map_err(|e| DatabaseError::ConnectionError(e.to_string()))?;
        if collection_names.is_empty() {
            let classes_collection = db.collection::<Document>("classes");
            let index = IndexModel::builder().keys(doc! { "name": 1 }).options(IndexOptions::builder().unique(true).build()).build();
            classes_collection.create_index(index).await.map_err(|e| DatabaseError::ConnectionError(e.to_string()))?;

            let rules_collection = db.collection::<Document>("rules");
            let index = IndexModel::builder().keys(doc! { "name": 1 }).options(IndexOptions::builder().unique(true).build()).build();
            rules_collection.create_index(index).await.map_err(|e| DatabaseError::ConnectionError(e.to_string()))?;

            let object_data_collection = db.collection::<Document>("object_data");
            let index = IndexModel::builder().keys(doc! { "object_id": 1, "timestamp": 1 }).options(IndexOptions::builder().unique(true).build()).build();
            object_data_collection.create_index(index).await.map_err(|e| DatabaseError::ConnectionError(e.to_string()))?;

            let fcm_tokens_collection = db.collection::<Document>("fcm_tokens");
            let index = IndexModel::builder().keys(doc! { "object_id": 1, "token": 1 }).options(IndexOptions::builder().unique(true).build()).build();
            fcm_tokens_collection.create_index(index).await.map_err(|e| DatabaseError::ConnectionError(e.to_string()))?;
        }
        Ok(Self { name: name.to_owned(), client })
    }
}

#[async_trait]
impl Database for MongoDB {
    fn name(&self) -> &str {
        &self.name
    }

    async fn create_class(&self, class: &Class) -> Result<(), DatabaseError> {
        let db = self.client.database(&self.name);
        let collection = db.collection::<Class>("classes");
        collection.insert_one(class).await.map_err(|e| if e.to_string().contains("duplicate key error") { DatabaseError::ClassAlreadyExists(class.name.clone()) } else { DatabaseError::ConnectionError(e.to_string()) })?;
        Ok(())
    }

    async fn add_class(&self, object_id: &str, class_name: &str) -> Result<(), DatabaseError> {
        let db = self.client.database(&self.name);
        let collection = db.collection::<MongoObject>("objects");
        let oid = ObjectId::parse_str(object_id).map_err(|e| DatabaseError::ConnectionError(e.to_string()))?;
        collection.update_one(doc! { "_id": oid }, doc! { "$addToSet": { "classes": class_name } }).await.map_err(|e| DatabaseError::ConnectionError(e.to_string()))?;
        Ok(())
    }

    async fn drop_database(&self) -> Result<(), DatabaseError> {
        self.client.database(&self.name).drop().await.map_err(|e| DatabaseError::ConnectionError(e.to_string()))?;
        Ok(())
    }
}

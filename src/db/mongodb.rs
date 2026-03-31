use crate::db::{Database, DatabaseError};
use crate::model::Class;
use async_trait::async_trait;
use futures::TryStreamExt;
use mongodb::bson::{self, doc};
use mongodb::{Client, IndexModel, bson::Document, options::IndexOptions};

#[derive(Clone)]
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

    async fn get_classes(&self) -> Result<Vec<Class>, DatabaseError> {
        let db = self.client.database(&self.name);
        let collection = db.collection::<Class>("classes");
        let cursor = collection.find(doc! {}).await.map_err(|e| DatabaseError::ConnectionError(e.to_string()))?;
        let classes: Vec<Class> = cursor.try_collect().await.map_err(|e| DatabaseError::ConnectionError(e.to_string()))?;
        Ok(classes)
    }
}

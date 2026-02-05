use crate::DataStore;
use async_trait::async_trait;
use mongodb::bson::doc;
use mongodb::error::Error;
use mongodb::options::IndexOptions;
use mongodb::{Client, IndexModel};

pub struct MongoDBDataStore {
    name: String,
    client: Client,
}

impl MongoDBDataStore {
    pub async fn new(name: &str, connection_string: &str) -> Result<Self, Error> {
        let client = Client::with_uri_str(connection_string).await?;
        let db = client.database(name);
        let collection_names = db.list_collection_names().await?;
        if collection_names.is_empty() {
            let classes_collection = db.collection::<mongodb::bson::Document>("classes");
            let index = IndexModel::builder().keys(doc! { "name": 1 }).options(IndexOptions::builder().unique(true).build()).build();
            classes_collection.create_index(index).await?;

            let rules_collection = db.collection::<mongodb::bson::Document>("rules");
            let index = IndexModel::builder().keys(doc! { "name": 1 }).options(IndexOptions::builder().unique(true).build()).build();
            rules_collection.create_index(index).await?;

            let object_data_collection = db.collection::<mongodb::bson::Document>("object_data");
            let index = IndexModel::builder().keys(doc! { "object_id": 1, "timestamp": 1 }).options(IndexOptions::builder().unique(true).build()).build();
            object_data_collection.create_index(index).await?;
        }
        Ok(Self { name: name.to_string(), client })
    }
}

#[async_trait]
impl DataStore for MongoDBDataStore {
    async fn add_class(&self, class_name: &str) -> Result<(), String> {
        // Implement the logic to add a class to MongoDB
        Ok(())
    }
}

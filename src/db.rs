use futures::TryStreamExt;
use mongodb::bson::doc;
use mongodb::bson::oid::ObjectId;
use mongodb::error::Result;
use mongodb::options::IndexOptions;
use mongodb::{Client, IndexModel};
use serde::{Deserialize, Serialize};

pub struct Database {
    name: String,
    client: Client,
}

#[derive(Serialize, Deserialize, Debug)]
pub struct DBKind {
    pub name: String,
}

#[derive(Serialize, Deserialize, Debug)]
pub struct DBItem {
    #[serde(rename = "_id", skip_serializing_if = "Option::is_none")]
    pub id: Option<ObjectId>,
    pub kinds: Vec<String>,
}

#[derive(Serialize, Deserialize, Debug)]
pub struct DBRule {
    pub name: String,
    pub content: String,
}

impl Database {
    pub async fn new(name: &str, connection_string: &str) -> Result<Self> {
        let client = Client::with_uri_str(connection_string).await?;
        let db = client.database(name);
        let collection_names = db.list_collection_names().await?;
        if collection_names.is_empty() {
            let types_collection = db.collection::<mongodb::bson::Document>("types");
            let index = IndexModel::builder()
                .keys(doc! { "name": 1 })
                .options(IndexOptions::builder().unique(true).build())
                .build();
            types_collection.create_index(index).await?;

            let rules_collection = db.collection::<mongodb::bson::Document>("rules");
            let index = IndexModel::builder()
                .keys(doc! { "name": 1 })
                .options(IndexOptions::builder().unique(true).build())
                .build();
            rules_collection.create_index(index).await?;

            let item_data_collection = db.collection::<mongodb::bson::Document>("item_data");
            let index = IndexModel::builder()
                .keys(doc! { "item_id": 1, "timestamp": 1 })
                .options(IndexOptions::builder().unique(true).build())
                .build();
            item_data_collection.create_index(index).await?;
        }
        Ok(Self {
            name: name.to_string(),
            client,
        })
    }

    pub async fn get_types(&self) -> Result<Vec<DBKind>> {
        let mut cursor = self
            .client
            .database(&self.name)
            .collection::<DBKind>("types")
            .find(doc! {})
            .await?;
        let mut kinds = Vec::new();
        while let Some(kind) = cursor.try_next().await? {
            kinds.push(kind);
        }
        Ok(kinds)
    }

    pub async fn get_items(&self) -> Result<Vec<DBItem>> {
        let mut cursor = self
            .client
            .database(&self.name)
            .collection::<DBItem>("items")
            .find(doc! {})
            .await?;
        let mut items = Vec::new();
        while let Some(item) = cursor.try_next().await? {
            items.push(item);
        }
        Ok(items)
    }

    pub async fn get_rules(&self) -> Result<Vec<DBRule>> {
        let mut cursor = self
            .client
            .database(&self.name)
            .collection::<DBRule>("rules")
            .find(doc! {})
            .await?;
        let mut rules = Vec::new();
        while let Some(rule) = cursor.try_next().await? {
            rules.push(rule);
        }
        Ok(rules)
    }
}

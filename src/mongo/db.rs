use async_trait::async_trait;
use futures::TryStreamExt;
use mongodb::bson::doc;
use mongodb::bson::oid::ObjectId;
use mongodb::options::IndexOptions;
use mongodb::{Client, IndexModel};
use serde::{Deserialize, Serialize};
use std::error::Error;

use crate::db::{DBItem, DBKind, DBRule, Database as DatabaseTrait};

#[derive(Serialize, Deserialize, Debug)]
struct MongoKind {
    pub name: String,
}

#[derive(Serialize, Deserialize, Debug)]
struct MongoItem {
    #[serde(rename = "_id", skip_serializing_if = "Option::is_none")]
    pub id: Option<ObjectId>,
    pub kinds: Vec<String>,
}

#[derive(Serialize, Deserialize, Debug)]
struct MongoRule {
    pub name: String,
    pub content: String,
}

pub struct Database {
    name: String,
    client: Client,
}

impl Database {
    pub async fn new(name: &str, connection_string: &str) -> Result<Self, Box<dyn Error>> {
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
}

#[async_trait]
impl DatabaseTrait for Database {
    fn name(&self) -> &str {
        &self.name
    }

    async fn get_types(&self) -> Result<Vec<DBKind>, Box<dyn Error>> {
        let mut cursor = self
            .client
            .database(&self.name)
            .collection::<MongoKind>("types")
            .find(doc! {})
            .await?;

        let mut kinds = Vec::new();
        while let Some(kind) = cursor.try_next().await? {
            kinds.push(DBKind { name: kind.name });
        }
        Ok(kinds)
    }

    async fn create_type(&self, kind: &DBKind) -> Result<(), Box<dyn Error>> {
        let collection = self
            .client
            .database(&self.name)
            .collection::<MongoKind>("types");
        let mongo_kind = MongoKind {
            name: kind.name.clone(),
        };
        collection.insert_one(mongo_kind).await?;
        Ok(())
    }

    async fn get_items(&self) -> Result<Vec<DBItem>, Box<dyn Error>> {
        let mut cursor = self
            .client
            .database(&self.name)
            .collection::<MongoItem>("items")
            .find(doc! {})
            .await?;

        let mut items = Vec::new();
        while let Some(item) = cursor.try_next().await? {
            items.push(DBItem {
                id: item.id.unwrap().to_hex(),
                kinds: item.kinds,
            });
        }
        Ok(items)
    }

    async fn create_item(&self, item: &DBItem) -> Result<(), Box<dyn Error>> {
        let collection = self
            .client
            .database(&self.name)
            .collection::<MongoItem>("items");
        let mongo_item = MongoItem {
            id: None,
            kinds: item.kinds.clone(),
        };
        collection.insert_one(mongo_item).await?;
        Ok(())
    }

    async fn get_rules(&self) -> Result<Vec<DBRule>, Box<dyn Error>> {
        let mut cursor = self
            .client
            .database(&self.name)
            .collection::<MongoRule>("rules")
            .find(doc! {})
            .await?;

        let mut rules = Vec::new();
        while let Some(rule) = cursor.try_next().await? {
            rules.push(DBRule {
                name: rule.name,
                content: rule.content,
            });
        }
        Ok(rules)
    }

    async fn create_rule(&self, rule: &DBRule) -> Result<(), Box<dyn Error>> {
        let collection = self
            .client
            .database(&self.name)
            .collection::<MongoRule>("rules");
        let mongo_rule = MongoRule {
            name: rule.name.clone(),
            content: rule.content.clone(),
        };
        collection.insert_one(mongo_rule).await?;
        Ok(())
    }

    async fn drop_db(&self) -> Result<(), Box<dyn Error>> {
        self.client.database(&self.name).drop().await?;
        Ok(())
    }
}

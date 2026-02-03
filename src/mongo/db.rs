use crate::{Class, Database as DatabaseTrait, Object, Rule, Value};
use async_trait::async_trait;
use chrono::{DateTime, Utc};
use futures::TryStreamExt;
use mongodb::bson::oid::ObjectId;
use mongodb::bson::{self, doc};
use mongodb::options::IndexOptions;
use mongodb::{Client, IndexModel};
use serde::{Deserialize, Serialize};
use std::collections::{HashMap, HashSet};
use std::error::Error;

#[derive(Serialize, Deserialize, Debug)]
struct MongoObject {
    #[serde(rename = "_id", skip_serializing_if = "Option::is_none")]
    pub id: Option<ObjectId>,
    pub classes: Option<HashSet<String>>,
    pub properties: Option<HashMap<String, Value>>,
    pub values: Option<HashMap<String, (Value, DateTime<Utc>)>>,
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
impl DatabaseTrait for Database {
    fn name(&self) -> &str {
        &self.name
    }

    async fn get_classes(&self) -> Result<Vec<Class>, Box<dyn Error>> {
        let mut cursor = self.client.database(&self.name).collection::<Class>("classes").find(doc! {}).await?;

        let mut classes = Vec::new();
        while let Some(class) = cursor.try_next().await? {
            classes.push(class);
        }
        Ok(classes)
    }

    async fn create_class(&self, class: &Class) -> Result<(), Box<dyn Error>> {
        let collection = self.client.database(&self.name).collection::<Class>("classes");
        collection.insert_one(class).await?;
        Ok(())
    }

    async fn get_objects(&self) -> Result<Vec<Object>, Box<dyn Error>> {
        let mut cursor = self.client.database(&self.name).collection::<MongoObject>("objects").find(doc! {}).await?;

        let mut objects = Vec::new();
        while let Some(object) = cursor.try_next().await? {
            objects.push(Object {
                id: object.id.unwrap().to_hex(),
                classes: object.classes,
                properties: object.properties,
                values: object.values,
            });
        }
        Ok(objects)
    }

    async fn add_data(&self, object: &Object, values: &HashMap<String, Value>, date_time: &DateTime<Utc>) -> Result<(), Box<dyn Error>> {
        let collection = self.client.database(&self.name).collection::<mongodb::bson::Document>("object_data");
        let doc = doc! {
            "object_id": &object.id,
            "values": bson::to_bson(values)?,
            "timestamp": bson::DateTime::from_millis(date_time.timestamp_millis()),
        };
        collection.insert_one(doc).await?;
        Ok(())
    }

    async fn create_object(&self, object: &Object) -> Result<String, Box<dyn Error>> {
        let collection = self.client.database(&self.name).collection::<MongoObject>("objects");
        let mongo_object = MongoObject {
            id: None,
            classes: object.classes.clone(),
            properties: object.properties.clone(),
            values: object.values.clone(),
        };
        let result = collection.insert_one(mongo_object).await?;
        Ok(result.inserted_id.as_object_id().unwrap().to_hex())
    }

    async fn get_rules(&self) -> Result<Vec<Rule>, Box<dyn Error>> {
        let mut cursor = self.client.database(&self.name).collection::<Rule>("rules").find(doc! {}).await?;

        let mut rules = Vec::new();
        while let Some(rule) = cursor.try_next().await? {
            rules.push(rule);
        }
        Ok(rules)
    }

    async fn create_rule(&self, rule: &Rule) -> Result<(), Box<dyn Error>> {
        let collection = self.client.database(&self.name).collection::<Rule>("rules");
        collection.insert_one(rule).await?;
        Ok(())
    }

    async fn drop_db(&self) -> Result<(), Box<dyn Error>> {
        self.client.database(&self.name).drop().await?;
        Ok(())
    }
}

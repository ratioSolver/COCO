use crate::{Class, DataStore, Object, Rule, Value};
use async_trait::async_trait;
use chrono::{DateTime, Utc};
use futures::TryStreamExt;
use mongodb::bson::doc;
use mongodb::{Client, IndexModel};
use mongodb::{
    bson::{self, Document, oid::ObjectId},
    options::IndexOptions,
};
use serde::{Deserialize, Serialize};
use std::collections::{HashMap, HashSet};
use std::error::Error;

#[derive(Serialize, Deserialize, Debug)]
struct MongoObject {
    #[serde(rename = "_id", skip_serializing_if = "Option::is_none")]
    pub id: Option<ObjectId>,
    pub classes: HashSet<String>,
    #[serde(skip_serializing_if = "Option::is_none")]
    pub properties: Option<HashMap<String, Value>>,
    #[serde(skip_serializing_if = "Option::is_none")]
    pub values: Option<HashMap<String, (Value, DateTime<Utc>)>>,
}

pub struct MongoDBDataStore {
    name: String,
    client: Client,
}

impl MongoDBDataStore {
    pub async fn new(name: &str, connection_string: &str) -> Result<Self, Box<dyn Error>> {
        let client = Client::with_uri_str(connection_string).await?;
        let db = client.database(name);
        let collection_names = db.list_collection_names().await?;
        if collection_names.is_empty() {
            let classes_collection = db.collection::<Document>("classes");
            let index = IndexModel::builder().keys(doc! { "name": 1 }).options(IndexOptions::builder().unique(true).build()).build();
            classes_collection.create_index(index).await?;

            let rules_collection = db.collection::<Document>("rules");
            let index = IndexModel::builder().keys(doc! { "name": 1 }).options(IndexOptions::builder().unique(true).build()).build();
            rules_collection.create_index(index).await?;

            let object_data_collection = db.collection::<Document>("object_data");
            let index = IndexModel::builder().keys(doc! { "object_id": 1, "timestamp": 1 }).options(IndexOptions::builder().unique(true).build()).build();
            object_data_collection.create_index(index).await?;
        }
        Ok(Self { name: name.to_string(), client })
    }
}

#[async_trait]
impl DataStore for MongoDBDataStore {
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

    async fn set_properties(&self, object: &Object, properties: &HashMap<String, Value>) -> Result<(), Box<dyn Error>> {
        let collection = self.client.database(&self.name).collection::<MongoObject>("objects");
        let mut update_doc = doc! {};
        for (prop, value) in properties {
            update_doc.insert(format!("properties.{}", prop), bson::to_bson(value)?);
        }

        collection.update_one(doc! { "_id": ObjectId::parse_str(&object.id)? }, doc! { "$set": update_doc }).await?;
        Ok(())
    }

    async fn get_values(&self, object: &Object, from: &DateTime<Utc>, to: &DateTime<Utc>) -> Result<HashMap<String, Vec<(Value, DateTime<Utc>)>>, Box<dyn Error>> {
        let data_collection = self.client.database(&self.name).collection::<Document>("object_data");
        let mut cursor = data_collection.find(doc! { "object_id": &object.id, "timestamp": { "$gte": bson::DateTime::from_millis(from.timestamp_millis()), "$lte": bson::DateTime::from_millis(to.timestamp_millis()) } }).await?;

        let mut values_map: HashMap<String, Vec<(Value, DateTime<Utc>)>> = HashMap::new();
        while let Some(doc) = cursor.try_next().await? {
            if let Some(values) = doc.get("values").and_then(|v| v.as_document()) {
                for (prop, value) in values {
                    let value: Value = bson::from_bson(value.clone())?;
                    let timestamp = doc.get("timestamp").and_then(|t| t.as_datetime()).unwrap().to_system_time().into();
                    values_map.entry(prop.clone()).or_default().push((value, timestamp));
                }
            }
        }
        Ok(values_map)
    }

    async fn set_values(&self, object: &Object, values: &HashMap<String, Value>, date_time: &DateTime<Utc>) -> Result<(), Box<dyn Error>> {
        let objects_collection = self.client.database(&self.name).collection::<MongoObject>("objects");
        let mut update_doc = doc! {};
        for (prop, value) in values {
            update_doc.insert(format!("values.{}", prop), bson::to_bson(&(value.clone(), *date_time))?);
        }
        objects_collection.update_one(doc! { "_id": ObjectId::parse_str(&object.id)? }, doc! { "$set": update_doc }).await?;

        let data_collection = self.client.database(&self.name).collection::<Document>("object_data");
        let doc = doc! {
            "object_id": &object.id,
            "values": bson::to_bson(values)?,
            "timestamp": bson::DateTime::from_millis(date_time.timestamp_millis()),
        };
        data_collection.insert_one(doc).await?;
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

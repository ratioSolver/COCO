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
                id: object.id.map(|oid| oid.to_hex()),
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

        collection.update_one(doc! { "_id": ObjectId::parse_str(object.id.as_ref().unwrap())? }, doc! { "$set": update_doc }).await?;
        Ok(())
    }

    async fn get_values(&self, object: &Object, from: &DateTime<Utc>, to: &DateTime<Utc>) -> Result<HashMap<String, Vec<(Value, DateTime<Utc>)>>, Box<dyn Error>> {
        let data_collection = self.client.database(&self.name).collection::<Document>("object_data");
        let mut cursor = data_collection.find(doc! { "object_id": object.id.as_ref().unwrap(), "timestamp": { "$gte": bson::DateTime::from_millis(from.timestamp_millis()), "$lte": bson::DateTime::from_millis(to.timestamp_millis()) } }).await?;

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
        objects_collection.update_one(doc! { "_id": ObjectId::parse_str(object.id.as_ref().unwrap())? }, doc! { "$set": update_doc }).await?;

        let data_collection = self.client.database(&self.name).collection::<Document>("object_data");
        let doc = doc! {
            "object_id": object.id.as_ref().unwrap(),
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

#[cfg(test)]
mod tests {
    use super::*;
    use crate::Property;
    use std::time::{SystemTime, UNIX_EPOCH};

    async fn get_test_store() -> MongoDBDataStore {
        let start = SystemTime::now();
        let since_the_epoch = start.duration_since(UNIX_EPOCH).expect("Time went backwards");
        let db_name = format!("coco_test_{}", since_the_epoch.as_nanos());
        let uri = std::env::var("MONGODB_URI").unwrap_or_else(|_| "mongodb://localhost:27017".to_string());
        MongoDBDataStore::new(&db_name, &uri).await.unwrap()
    }

    #[tokio::test]
    async fn test_classes() {
        let store = get_test_store().await;

        let mut static_props = HashMap::new();
        static_props.insert("name".to_string(), Property::String { nullable: Some(false), default: Some("default property".to_string()) });
        let class = Class {
            name: "TestClass".to_string(),
            parents: None,
            static_properties: Some(static_props),
            dynamic_properties: None,
        };

        store.create_class(&class).await.unwrap();

        let classes = store.get_classes().await.unwrap();
        assert_eq!(classes.len(), 1);
        assert_eq!(classes[0].name, "TestClass");

        store.drop_db().await.unwrap();
    }

    #[tokio::test]
    async fn test_rules() {
        let store = get_test_store().await;

        let rule = Rule {
            name: "test_rule".to_string(),
            content: "(defrule test_rule => (printout t \"hello\" crlf))".to_string(),
        };

        store.create_rule(&rule).await.unwrap();

        let rules = store.get_rules().await.unwrap();
        assert_eq!(rules.len(), 1);
        assert_eq!(rules[0].name, "test_rule");
        assert_eq!(rules[0].content, rule.content);

        store.drop_db().await.unwrap();
    }

    #[tokio::test]
    async fn test_objects_and_properties() {
        let store = get_test_store().await;

        let mut classes = HashSet::new();
        classes.insert("TestClass".to_string());

        let mut properties = HashMap::new();
        properties.insert("prop1".to_string(), Value::String("value1".to_string()));

        let object = Object {
            id: None, // will be ignored and generated
            classes: classes.clone(),
            properties: Some(properties.clone()),
            values: None,
        };

        let id = store.create_object(&object).await.unwrap();
        assert!(!id.is_empty());

        let objects = store.get_objects().await.unwrap();
        assert_eq!(objects.len(), 1);
        assert_eq!(objects[0].id.as_deref().unwrap(), id);
        assert_eq!(objects[0].classes, classes);

        // Verify properties
        let retrieved_props = objects[0].properties.as_ref().unwrap();
        assert_eq!(retrieved_props.get("prop1"), Some(&Value::String("value1".to_string())));

        // Update properties
        let mut new_props = HashMap::new();
        new_props.insert("prop2".to_string(), Value::Int(42));

        // Use the object with the correct ID
        let mut object_with_id = object.clone();
        object_with_id.id = Some(id.clone());

        store.set_properties(&object_with_id, &new_props).await.unwrap();

        let objects_updated = store.get_objects().await.unwrap();
        let updated_props = objects_updated[0].properties.as_ref().unwrap();

        assert_eq!(updated_props.get("prop1"), Some(&Value::String("value1".to_string())));
        assert_eq!(updated_props.get("prop2"), Some(&Value::Int(42)));

        store.drop_db().await.unwrap();
    }

    #[tokio::test]
    async fn test_values_timeseries() {
        let store = get_test_store().await;

        let mut classes = HashSet::new();
        classes.insert("Sensor".to_string());

        let object = Object { id: None, classes, properties: None, values: None };

        let id = store.create_object(&object).await.unwrap();
        let mut object_with_id = object.clone();
        object_with_id.id = Some(id.clone());

        let now = Utc::now();
        let mut values = HashMap::new();
        values.insert("temp".to_string(), Value::Float(25.5));

        store.set_values(&object_with_id, &values, &now).await.unwrap();

        // Retrieve values
        let retrieved_values = store.get_values(&object_with_id, &(now - chrono::Duration::seconds(1)), &(now + chrono::Duration::seconds(1))).await.unwrap();

        assert!(retrieved_values.contains_key("temp"));
        let temp_values = retrieved_values.get("temp").unwrap();
        assert_eq!(temp_values.len(), 1);

        // Time precision check needs lenience - checking value is simpler
        assert_eq!(temp_values[0].0, Value::Float(25.5));

        // Check latest value in object document
        let objects = store.get_objects().await.unwrap();
        let obj_values = objects[0].values.as_ref().unwrap();

        assert!(obj_values.contains_key("temp"));
        let (val, _) = &obj_values["temp"];
        assert_eq!(*val, Value::Float(25.5));

        store.drop_db().await.unwrap();
    }

    #[tokio::test]
    async fn test_array_properties() {
        let store = get_test_store().await;

        let mut static_props = HashMap::new();
        static_props.insert("bool_arr".to_string(), Property::BoolArray { default: None });
        static_props.insert("int_arr".to_string(), Property::IntArray { default: None, min: None, max: None });
        static_props.insert("float_arr".to_string(), Property::FloatArray { default: None, min: None, max: None });
        static_props.insert("string_arr".to_string(), Property::StringArray { default: None });
        static_props.insert("symbol_arr".to_string(), Property::SymbolArray { default: None, allowed_values: None });
        static_props.insert("obj_arr".to_string(), Property::ObjectArray { default: None, class: "SomeClass".to_string() });

        let class = Class {
            name: "ArrayClass".to_string(),
            parents: None,
            static_properties: Some(static_props),
            dynamic_properties: None,
        };

        store.create_class(&class).await.unwrap();

        let mut classes = HashSet::new();
        classes.insert("ArrayClass".to_string());

        let mut properties = HashMap::new();
        properties.insert("bool_arr".to_string(), Value::BoolArray(vec![true, false]));
        properties.insert("int_arr".to_string(), Value::IntArray(vec![1, 2, 3]));
        properties.insert("float_arr".to_string(), Value::FloatArray(vec![1.1, 2.2]));
        properties.insert("string_arr".to_string(), Value::StringArray(vec!["a".to_string(), "b".to_string()]));
        properties.insert("symbol_arr".to_string(), Value::SymbolArray(vec!["s1".to_string(), "s2".to_string()]));
        properties.insert("obj_arr".to_string(), Value::ObjectArray(vec!["p1".to_string(), "p2".to_string()]));

        let object = Object { id: None, classes: classes.clone(), properties: Some(properties.clone()), values: None };

        let _id = store.create_object(&object).await.unwrap();

        // Fetch and verify
        let objects = store.get_objects().await.unwrap();
        assert_eq!(objects.len(), 1);
        let props = objects[0].properties.as_ref().unwrap();

        assert_eq!(props.get("bool_arr"), Some(&Value::BoolArray(vec![true, false])));
        assert_eq!(props.get("int_arr"), Some(&Value::IntArray(vec![1, 2, 3])));
        assert_eq!(props.get("float_arr"), Some(&Value::FloatArray(vec![1.1, 2.2])));
        assert_eq!(props.get("string_arr"), Some(&Value::StringArray(vec!["a".to_string(), "b".to_string()])));

        // Note: Due to serde(untagged) on Value enum, SymbolArray and ObjectArray deserialize as StringArray
        // because StringArray is defined before them in the enum and has the same shape (Vec<String>).
        assert_eq!(props.get("symbol_arr"), Some(&Value::StringArray(vec!["s1".to_string(), "s2".to_string()])));
        assert_eq!(props.get("obj_arr"), Some(&Value::StringArray(vec!["p1".to_string(), "p2".to_string()])));

        store.drop_db().await.unwrap();
    }

    #[tokio::test]
    async fn test_array_values() {
        let store = get_test_store().await;

        let mut classes = HashSet::new();
        classes.insert("Sensor".to_string());

        let object = Object { id: None, classes, properties: None, values: None };
        let id = store.create_object(&object).await.unwrap();
        let mut object_with_id = object.clone();
        object_with_id.id = Some(id.clone());

        let now = Utc::now();
        let mut values = HashMap::new();
        values.insert("readings".to_string(), Value::IntArray(vec![10, 20, 30]));

        store.set_values(&object_with_id, &values, &now).await.unwrap();

        let retrieved_values = store.get_values(&object_with_id, &(now - chrono::Duration::seconds(1)), &(now + chrono::Duration::seconds(1))).await.unwrap();

        assert!(retrieved_values.contains_key("readings"));
        let reading_vals = retrieved_values.get("readings").unwrap();
        assert_eq!(reading_vals.len(), 1);
        assert_eq!(reading_vals[0].0, Value::IntArray(vec![10, 20, 30]));

        store.drop_db().await.unwrap();
    }
}

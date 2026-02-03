use chrono::{DateTime, Utc};

use crate::{Class, Database, KnowledgeBase, Object, Property, Rule, Value};
use std::{
    collections::{HashMap, HashSet},
    error::Error,
    sync::{Arc, Mutex, RwLock},
};

pub struct CoCo<DB: Database + Send + Sync, KB: KnowledgeBase + Send> {
    db: DB,
    kb: Mutex<KB>,
    classes: HashMap<String, Class>,
    objects: Arc<RwLock<HashMap<String, Object>>>,
    rules: HashMap<String, Rule>,
}

impl<DB: Database + Send + Sync, KB: KnowledgeBase + Send> CoCo<DB, KB> {
    pub async fn new(db: DB, kb: KB) -> Self {
        let mut coco = Self {
            db,
            kb: Mutex::new(kb),
            classes: HashMap::new(),
            objects: Arc::new(RwLock::new(HashMap::new())),
            rules: HashMap::new(),
        };

        let objects = coco.objects.clone();
        coco.kb.lock().unwrap().set_data_callback(move |object_id: &str, properties: HashMap<String, Value>, timestamp: DateTime<Utc>| {
            let mut map = objects.write().expect("Failed to lock objects for writing");
            let object = map.get_mut(object_id).expect("Object not found in callback");
            for (prop, value) in properties {
                object.values.get_or_insert_with(HashMap::new).insert(prop, (value, timestamp));
            }
        });

        coco.add_classes(coco.db.get_classes().await.unwrap());
        coco.add_rules(coco.db.get_rules().await.unwrap());
        coco.add_objects(coco.db.get_objects().await.unwrap());

        coco
    }

    pub fn get_classes(&self) -> Vec<Class> {
        self.classes.values().cloned().collect()
    }

    pub fn get_class(&self, name: &str) -> Option<Class> {
        self.classes.get(name).cloned()
    }

    pub async fn create_class(&mut self, name: &str, parents: Option<HashSet<String>>, static_properties: Option<HashMap<String, Property>>, dynamic_properties: Option<HashMap<String, Property>>) {
        let class = Class { name: name.to_string(), parents, static_properties, dynamic_properties };
        self.db.create_class(&class).await.expect("Failed to create class in database");
        self.add_classes(vec![class]);
    }

    fn add_classes(&mut self, classes: Vec<Class>) {
        for class in classes {
            self.kb.lock().unwrap().create_class(&class).expect("Failed to create class in knowledge base");
            self.classes.insert(class.name.clone(), class);
        }
    }

    pub async fn create_object(&mut self, classes: Option<HashSet<String>>, properties: Option<HashMap<String, Value>>, values: Option<HashMap<String, (Value, DateTime<Utc>)>>) {
        let mut object = Object { id: String::new(), classes, properties, values };
        object.id = self.db.create_object(&object).await.expect("Failed to create object in database");
        self.add_objects(vec![object]);
    }

    fn add_objects(&mut self, objects: Vec<Object>) {
        for object in objects {
            for class_name in object.classes.iter().flatten() {
                let class = self.classes.get(class_name).expect("Class not found for object");
                self.kb.lock().unwrap().create_object(class, &object).expect("Failed to create object in knowledge base");
            }
            self.objects.write().expect("Failed to lock objects for writing").insert(object.id.clone(), object);
        }
    }

    pub fn get_object(&self, id: &str) -> Option<Object> {
        self.objects.read().unwrap().get(id).cloned()
    }

    pub fn get_rule(&self, name: &str) -> Option<&Rule> {
        self.rules.get(name)
    }

    pub async fn create_rule(&mut self, name: &str, content: &str) {
        let rule = Rule { name: name.to_string(), content: content.to_string() };
        self.db.create_rule(&rule).await.expect("Failed to create rule in database");
        self.add_rules(vec![rule]);
    }

    fn add_rules(&mut self, db_rules: Vec<Rule>) {
        for rule in db_rules {
            self.rules.insert(rule.name.clone(), rule);
        }
    }

    pub async fn drop_db(&self) -> Result<(), Box<dyn Error>> {
        self.db.drop_db().await
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::clips::kb::KnowledgeBase as CLIPS;
    use crate::mongo::db::Database as MongoDB;

    #[tokio::test]
    async fn test_coco_initialization() {
        let coco = CoCo::new(MongoDB::new("test_coco_initialization", "mongodb://localhost:27017").await.unwrap(), CLIPS::new()).await;

        coco.drop_db().await.unwrap();
    }

    #[tokio::test]
    async fn test_create_class() {
        let mut coco = CoCo::new(MongoDB::new("coco_test_create_class", "mongodb://localhost:27017").await.unwrap(), CLIPS::new()).await;

        coco.create_class("TestClass", None, None, None).await;

        let class = coco.get_class("TestClass");
        assert!(class.is_some());
        assert_eq!(class.unwrap().name, "TestClass");

        coco.drop_db().await.unwrap();
    }

    #[tokio::test]
    async fn test_create_class_with_properties() {
        let mut coco = CoCo::new(MongoDB::new("coco_test_create_class_with_properties", "mongodb://localhost:27017").await.unwrap(), CLIPS::new()).await;

        let mut static_props = HashMap::new();
        static_props.insert("is_active".to_string(), Property::Bool { nullable: Some(false), default: Some(false) });

        let mut dynamic_props = HashMap::new();
        dynamic_props.insert("temperature".to_string(), Property::Float { nullable: Some(false), default: None, min: Some(-50.0), max: Some(150.0) });

        coco.create_class("Sensor", None, Some(static_props), Some(dynamic_props)).await;

        let class = coco.get_class("Sensor");
        assert!(class.is_some());
        let class = class.unwrap();
        assert_eq!(class.name, "Sensor");
        assert!(class.static_properties.is_some());
        assert!(class.dynamic_properties.is_some());
        assert!(class.static_properties.as_ref().unwrap().contains_key("is_active"));
        assert!(class.dynamic_properties.as_ref().unwrap().contains_key("temperature"));

        coco.drop_db().await.unwrap();
    }
}

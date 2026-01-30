use crate::db::{Class, Database, DynamicValue, Object, Property, Rule, StaticValue};
use std::{
    collections::{HashMap, HashSet},
    error::Error,
    sync::{Arc, RwLock},
};

pub struct CoCo {
    db: Box<dyn Database + Send + Sync>,
    classes: HashMap<String, Class>,
    objects: Arc<RwLock<HashMap<String, Arc<RwLock<Object>>>>>,
    rules: HashMap<String, Rule>,
}

impl CoCo {
    pub async fn new(db: Box<dyn Database + Send + Sync>) -> Self {
        let objects = Arc::new(RwLock::new(HashMap::new()));
        let mut coco = Self {
            db,
            classes: HashMap::new(),
            objects: objects.clone(),
            rules: HashMap::new(),
        };

        coco.add_classes(coco.db.get_classes().await.unwrap());
        coco.add_rules(coco.db.get_rules().await.unwrap());
        coco.add_objects(coco.db.get_objects().await.unwrap());

        coco
    }

    pub fn get_class(&self, name: &str) -> Option<&Class> {
        self.classes.get(name)
    }

    pub async fn create_class(
        &mut self,
        name: &str,
        parents: Option<HashSet<String>>,
        static_properties: Option<HashMap<String, Property>>,
        dynamic_properties: Option<HashMap<String, Property>>,
    ) {
        let class = Class {
            name: name.to_string(),
            parents,
            static_properties,
            dynamic_properties,
        };
        self.db
            .create_class(&class)
            .await
            .expect("Failed to create class in database");
        self.add_classes(vec![class]);
    }

    fn add_classes(&mut self, classes: Vec<Class>) {
        for class in classes {
            self.classes.insert(class.name.to_string(), class);
        }
    }

    pub async fn create_object(
        &mut self,
        id: &str,
        classes: Option<HashSet<String>>,
        properties: Option<HashMap<String, StaticValue>>,
        values: Option<HashMap<String, DynamicValue>>,
    ) {
        let object = Object {
            id: id.to_string(),
            classes,
            properties,
            values,
        };
        self.db
            .create_object(&object)
            .await
            .expect("Failed to create object in database");
        self.add_objects(vec![object]);
    }

    fn add_objects(&mut self, objects: Vec<Object>) {
        for object in objects {
            let object_arc = Arc::new(RwLock::new(object));
            let id = object_arc.read().expect("Failed to lock object").id.clone();
            self.objects
                .write()
                .expect("Failed to lock objects map")
                .insert(id, object_arc.clone());
            for class_name in object_arc
                .read()
                .expect("Failed to lock object")
                .classes
                .iter()
                .flatten()
            {
                let class = self
                    .classes
                    .get(class_name)
                    .expect("Class not found for object");
            }
        }
    }

    pub fn get_object(&self, id: &str) -> Option<Arc<RwLock<Object>>> {
        self.objects
            .read()
            .expect("Failed to lock objects map")
            .get(id)
            .cloned()
    }

    pub fn get_rule(&self, name: &str) -> Option<&Rule> {
        self.rules.get(name)
    }

    pub async fn create_rule(&mut self, name: &str, content: &str) {
        let rule = Rule {
            name: name.to_string(),
            content: content.to_string(),
        };
        self.db
            .create_rule(&rule)
            .await
            .expect("Failed to create rule in database");
        self.add_rules(vec![rule]);
    }

    fn add_rules(&mut self, db_rules: Vec<Rule>) {
        for rule in db_rules {
            self.rules.insert(rule.name.to_string(), rule);
        }
    }

    pub async fn drop_db(&self) -> Result<(), Box<dyn Error>> {
        self.db.drop_db().await
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::mongo::db::Database as MongoDB;
    use std::collections::HashMap;

    #[tokio::test]
    async fn test_coco_initialization() {
        let coco = CoCo::new(Box::new(
            MongoDB::new("test_coco_initialization", "mongodb://localhost:27017")
                .await
                .unwrap(),
        ))
        .await;

        coco.drop_db().await.unwrap();
    }

    #[tokio::test]
    async fn test_create_class() {
        let mut coco = CoCo::new(Box::new(
            MongoDB::new("coco_test_create_class", "mongodb://localhost:27017")
                .await
                .unwrap(),
        ))
        .await;

        coco.create_class("TestClass", None, None, None).await;

        let class = coco.get_class("TestClass");
        assert!(class.is_some());
        assert_eq!(class.unwrap().name, "TestClass");

        coco.drop_db().await.unwrap();
    }

    #[tokio::test]
    async fn test_create_class_with_properties() {
        let mut coco = CoCo::new(Box::new(
            MongoDB::new(
                "coco_test_create_class_with_properties",
                "mongodb://localhost:27017",
            )
            .await
            .unwrap(),
        ))
        .await;

        let mut static_props = HashMap::new();
        static_props.insert(
            "is_active".to_string(),
            Property::Bool {
                name: "is_active".to_string(),
                required: Some(true),
                default: Some(false),
            },
        );

        let mut dynamic_props = HashMap::new();
        dynamic_props.insert(
            "temperature".to_string(),
            Property::Float {
                name: "temperature".to_string(),
                required: Some(true),
                default: None,
                min: Some(-50.0),
                max: Some(150.0),
            },
        );

        coco.create_class("Sensor", None, Some(static_props), Some(dynamic_props))
            .await;

        let class = coco.get_class("Sensor");
        assert!(class.is_some());
        let class = class.unwrap();
        assert_eq!(class.name, "Sensor");
        assert!(class.static_properties.is_some());
        assert!(class.dynamic_properties.is_some());
        assert!(
            class
                .static_properties
                .as_ref()
                .unwrap()
                .contains_key("is_active")
        );
        assert!(
            class
                .dynamic_properties
                .as_ref()
                .unwrap()
                .contains_key("temperature")
        );

        coco.drop_db().await.unwrap();
    }
}

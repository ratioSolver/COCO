use crate::{Class, CoCoEvent, DataStore, KnowledgeBase, Object, Property, Rule, Value};
use chrono::{DateTime, Utc};
use std::{
    collections::{HashMap, HashSet},
    error::Error,
    sync::{Arc, Mutex, RwLock},
};
use tokio::sync::broadcast;

pub struct CoCo {
    sender: broadcast::Sender<CoCoEvent>,
    db: Arc<dyn DataStore>,
    kb: Arc<Mutex<dyn KnowledgeBase>>,
    classes: RwLock<HashMap<String, Class>>,
    objects: RwLock<HashMap<String, Object>>,
    rules: RwLock<HashMap<String, Rule>>,
}

impl CoCo {
    pub async fn new(db: Arc<dyn DataStore>, kb: Arc<Mutex<dyn KnowledgeBase>>) -> Self {
        let sender = kb.lock().unwrap().get_event_sender();
        let coco = Self {
            sender,
            db: db.clone(),
            kb: kb.clone(),
            classes: RwLock::new(HashMap::new()),
            objects: RwLock::new(HashMap::new()),
            rules: RwLock::new(HashMap::new()),
        };
        let mut receiver = coco.sender.subscribe();
        tokio::spawn(async move {
            while let Ok(event) = receiver.recv().await {
                if let CoCoEvent::AddedValues(object_id, values) = event {
                    println!("Added values to object {}: {:?}", object_id, values);
                }
            }
        });
        coco
    }

    pub fn get_event_sender(&self) -> broadcast::Sender<CoCoEvent> {
        self.sender.clone()
    }

    pub fn get_classes(&self) -> Vec<Class> {
        self.classes.read().unwrap().values().cloned().collect()
    }

    pub fn get_class(&self, name: &str) -> Option<Class> {
        self.classes.read().unwrap().get(name).cloned()
    }

    pub async fn create_new_class(&self, name: &str, parents: Option<HashSet<String>>, static_properties: Option<HashMap<String, Property>>, dynamic_properties: Option<HashMap<String, Property>>) -> Result<(), Box<dyn Error>> {
        self.create_class(Class { name: name.to_string(), parents, static_properties, dynamic_properties }).await
    }

    pub async fn create_class(&self, class: Class) -> Result<(), Box<dyn Error>> {
        self.db.create_class(&class).await.expect("Failed to create class in database");
        self.add_classes(vec![class])?;
        Ok(())
    }

    fn add_classes(&self, classes: Vec<Class>) -> Result<(), Box<dyn Error>> {
        for class in classes {
            self.kb.lock().unwrap().create_class(&class)?;
            self.classes.write().unwrap().insert(class.name.clone(), class.clone());
            self.sender.send(CoCoEvent::ClassCreated(class)).expect("Failed to send ClassCreated event");
        }
        Ok(())
    }

    pub fn get_objects(&self) -> Vec<Object> {
        self.objects.read().unwrap().values().cloned().collect()
    }

    pub fn get_object(&self, id: &str) -> Option<Object> {
        self.objects.read().unwrap().get(id).cloned()
    }

    pub async fn create_new_object(&self, classes: HashSet<String>, properties: Option<HashMap<String, Value>>, values: Option<HashMap<String, (Value, DateTime<Utc>)>>) -> Result<(), Box<dyn Error>> {
        self.create_object(Object { id: None, classes, properties, values }).await
    }

    pub async fn create_object(&self, object: Object) -> Result<(), Box<dyn Error>> {
        let id = self.db.create_object(&object).await?;
        let object = Object { id: Some(id), ..object };
        self.add_objects(vec![object])?;
        Ok(())
    }

    fn add_objects(&self, objects: Vec<Object>) -> Result<(), Box<dyn Error>> {
        let class_guard = self.classes.read().unwrap();
        for object in objects {
            for class_name in &object.classes {
                let class = class_guard.get(class_name).expect("Class not found for object");
                self.kb.lock().unwrap().create_object(class, &object).expect("Failed to create object in knowledge base");
            }
            self.objects.write().expect("Failed to lock objects for writing").insert(object.id.clone().unwrap(), object.clone());
            self.sender.send(CoCoEvent::ObjectCreated(object)).expect("Failed to send ObjectCreated event");
        }
        Ok(())
    }

    pub async fn set_values(&self, object: &mut Object, values: &HashMap<String, Value>) -> Result<(), Box<dyn Error>> {
        let date_time: DateTime<Utc> = Utc::now();
        self.db.set_values(object, values, &date_time).await?;
        let class_guard = self.classes.read().unwrap();
        for class_name in &object.classes {
            let class = class_guard.get(class_name).expect("Class not found for object");
            self.kb.lock().unwrap().add_data(class, object, values, &date_time)?;
        }
        Ok(())
    }

    pub fn get_rules(&self) -> Vec<Rule> {
        self.rules.read().unwrap().values().cloned().collect()
    }

    pub fn get_rule(&self, name: &str) -> Option<Rule> {
        self.rules.read().unwrap().get(name).cloned()
    }

    pub async fn create_new_rule(&self, name: &str, content: &str) -> Result<(), Box<dyn Error>> {
        self.create_rule(Rule { name: name.to_string(), content: content.to_string() }).await
    }

    pub async fn create_rule(&self, rule: Rule) -> Result<(), Box<dyn Error>> {
        self.db.create_rule(&rule).await?;
        self.add_rules(vec![rule])?;
        Ok(())
    }

    fn add_rules(&self, db_rules: Vec<Rule>) -> Result<(), Box<dyn Error>> {
        for rule in db_rules {
            self.kb.lock().unwrap().create_rule(&rule)?;
            self.rules.write().unwrap().insert(rule.name.clone(), rule);
        }
        Ok(())
    }

    pub async fn drop_db(&self) -> Result<(), Box<dyn Error>> {
        self.db.drop_db().await?;
        Ok(())
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::{Class, CoCoEvent, DataStore, KnowledgeBase, Object, Property, Rule, Value};
    use async_trait::async_trait;
    use chrono::{DateTime, Utc};
    use std::collections::{HashMap, HashSet};
    use std::error::Error;
    use std::sync::{Arc, Mutex};
    use tokio::sync::broadcast;

    struct MockDataStore {
        classes: Arc<Mutex<HashMap<String, Class>>>,
        objects: Arc<Mutex<HashMap<String, Object>>>,
        rules: Arc<Mutex<HashMap<String, Rule>>>,
    }

    impl MockDataStore {
        fn new() -> Self {
            Self {
                classes: Arc::new(Mutex::new(HashMap::new())),
                objects: Arc::new(Mutex::new(HashMap::new())),
                rules: Arc::new(Mutex::new(HashMap::new())),
            }
        }
    }

    #[async_trait]
    impl DataStore for MockDataStore {
        fn name(&self) -> &str {
            "mock"
        }

        async fn get_classes(&self) -> Result<Vec<Class>, Box<dyn Error>> {
            Ok(self.classes.lock().unwrap().values().cloned().collect())
        }

        async fn create_class(&self, class: &Class) -> Result<(), Box<dyn Error>> {
            self.classes.lock().unwrap().insert(class.name.clone(), class.clone());
            Ok(())
        }

        async fn get_objects(&self) -> Result<Vec<Object>, Box<dyn Error>> {
            Ok(self.objects.lock().unwrap().values().cloned().collect())
        }

        async fn create_object(&self, object: &Object) -> Result<String, Box<dyn Error>> {
            let id = if let Some(ref obj_id) = object.id { obj_id.clone() } else { "mock_id".to_string() };
            let mut obj = object.clone();
            obj.id = Some(id.clone());
            self.objects.lock().unwrap().insert(id.clone(), obj);
            Ok(id)
        }

        async fn set_properties(&self, object: &Object, properties: &HashMap<String, Value>) -> Result<(), Box<dyn Error>> {
            let mut objects = self.objects.lock().unwrap();
            objects.get_mut(object.id.as_ref().unwrap()).unwrap().properties = Some(properties.clone());
            Ok(())
        }

        async fn get_values(&self, _object: &Object, _from: &DateTime<Utc>, _to: &DateTime<Utc>) -> Result<HashMap<String, Vec<(Value, DateTime<Utc>)>>, Box<dyn Error>> {
            Ok(HashMap::new())
        }

        async fn set_values(&self, object: &Object, values: &HashMap<String, Value>, date_time: &DateTime<Utc>) -> Result<(), Box<dyn Error>> {
            let mut objects = self.objects.lock().unwrap();
            objects.get_mut(object.id.as_ref().unwrap()).unwrap().values = Some(values.iter().map(|(k, v)| (k.clone(), (v.clone(), *date_time))).collect());
            Ok(())
        }

        async fn get_rules(&self) -> Result<Vec<Rule>, Box<dyn Error>> {
            Ok(self.rules.lock().unwrap().values().cloned().collect())
        }

        async fn create_rule(&self, rule: &Rule) -> Result<(), Box<dyn Error>> {
            self.rules.lock().unwrap().insert(rule.name.clone(), rule.clone());
            Ok(())
        }

        async fn drop_db(&self) -> Result<(), Box<dyn Error>> {
            Ok(())
        }
    }

    struct MockKnowledgeBase {
        sender: broadcast::Sender<CoCoEvent>,
    }

    impl MockKnowledgeBase {
        fn new() -> Self {
            let (sender, _) = broadcast::channel(100);
            Self { sender }
        }
    }

    impl KnowledgeBase for MockKnowledgeBase {
        fn get_event_sender(&self) -> broadcast::Sender<CoCoEvent> {
            self.sender.clone()
        }
        fn create_class(&self, _class: &Class) -> Result<(), Box<dyn Error>> {
            Ok(())
        }
        fn create_object(&self, _class: &Class, _object: &Object) -> Result<(), Box<dyn Error>> {
            Ok(())
        }
        fn set_properties(&self, _class: &Class, _object: &Object, _values: &HashMap<String, Value>) -> Result<(), Box<dyn Error>> {
            Ok(())
        }
        fn add_data(&self, _class: &Class, _object: &Object, _values: &HashMap<String, Value>, _date_time: &DateTime<Utc>) -> Result<(), Box<dyn Error>> {
            Ok(())
        }
        fn create_rule(&self, _rule: &Rule) -> Result<(), Box<dyn Error>> {
            Ok(())
        }
    }

    async fn setup_coco() -> CoCo {
        let db = Arc::new(MockDataStore::new());
        let kb = Arc::new(Mutex::new(MockKnowledgeBase::new()));
        CoCo::new(db, kb).await
    }

    #[tokio::test]
    async fn test_new() {
        let coco = setup_coco().await;
        assert!(coco.get_classes().is_empty());
        assert!(coco.get_objects().is_empty());
        assert!(coco.get_rules().is_empty());
    }

    #[tokio::test]
    async fn test_create_and_get_class() {
        let coco = setup_coco().await;
        let mut static_props = HashMap::new();
        static_props.insert("name".to_string(), Property::String { nullable: Some(false), default: Some("default".to_string()) });

        coco.create_new_class("TestClass", None, Some(static_props), None).await.expect("Failed to create class");

        let classes = coco.get_classes();
        assert_eq!(classes.len(), 1);
        assert_eq!(classes[0].name, "TestClass");

        let retrieved_class = coco.get_class("TestClass");
        assert!(retrieved_class.is_some());
        assert_eq!(retrieved_class.unwrap().name, "TestClass");
    }

    #[tokio::test]
    async fn test_create_and_get_object() {
        let coco = setup_coco().await;

        // Need a class first
        coco.create_new_class("TestClass", None, None, None).await.expect("Failed to create class");

        let mut classes = HashSet::new();
        classes.insert("TestClass".to_string());

        let mut properties = HashMap::new();
        properties.insert("prop1".to_string(), Value::String("val1".to_string()));

        coco.create_new_object(classes, Some(properties), None).await.expect("Failed to create object");

        let objects = coco.get_objects();
        assert_eq!(objects.len(), 1);
        assert_eq!(objects[0].id.as_deref().unwrap(), "mock_id");

        let retrieved_object = coco.get_object("mock_id");
        assert!(retrieved_object.is_some());
        assert_eq!(retrieved_object.unwrap().id.as_deref().unwrap(), "mock_id");
    }

    #[tokio::test]
    async fn test_set_values() {
        let coco = setup_coco().await;

        coco.create_new_class("Sensor", None, None, None).await.expect("Failed to create class");

        let mut classes = HashSet::new();
        classes.insert("Sensor".to_string());

        coco.create_new_object(classes, None, None).await.expect("Failed to create object");
        let mut object = coco.get_object("mock_id").expect("Object not found");

        let mut values = HashMap::new();
        values.insert("temp".to_string(), Value::Float(20.5));

        coco.set_values(&mut object, &values).await.expect("Failed to set values");
    }

    #[tokio::test]
    async fn test_create_and_get_rule() {
        let coco = setup_coco().await;

        coco.create_new_rule("test_rule", "(defrule ...)").await.expect("Failed to create rule");

        let rules = coco.get_rules();
        assert_eq!(rules.len(), 1);
        assert_eq!(rules[0].name, "test_rule");
        assert_eq!(rules[0].content, "(defrule ...)");

        let retrieved_rule = coco.get_rule("test_rule");
        assert!(retrieved_rule.is_some());
    }

    #[tokio::test]
    async fn test_events() {
        let coco = setup_coco().await;
        let mut receiver = coco.get_event_sender().subscribe();

        coco.create_new_class("EventClass", None, None, None).await.expect("Failed to create class");

        if let Ok(event) = receiver.recv().await {
            match event {
                CoCoEvent::ClassCreated(c) => assert_eq!(c.name, "EventClass"),
                _ => panic!("Wrong event type"),
            }
        } else {
            panic!("Failed to receive event");
        }
    }
}

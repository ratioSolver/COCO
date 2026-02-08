use chrono::Utc;
use coco::{CLIPSKnowledgeBase, Class, CoCo, DataStore, MongoDBDataStore, Object, Property, Rule, Value};
use std::collections::{HashMap, HashSet};
use std::sync::{Arc, Mutex};
use tokio::sync::broadcast;
use tokio::time::{Duration, sleep};

async fn setup_db(db_name: &str) -> Arc<MongoDBDataStore> {
    let mongo_uri = std::env::var("MONGO_URI").unwrap_or_else(|_| "mongodb://localhost:27017".to_string());
    let db = MongoDBDataStore::new(db_name, &mongo_uri).await.expect("Failed to connect to Mongo");
    db.drop_db().await.expect("Failed to drop old DB");
    Arc::new(db)
}

// Function to create KB correctly wrapped in Arc<Mutex>
fn create_kb() -> Arc<Mutex<dyn coco::KnowledgeBase>> {
    let (tx, _) = broadcast::channel(100);
    let kb_arc = CLIPSKnowledgeBase::new(tx);
    kb_arc
}

#[tokio::test]
async fn test_coco_initialization() {
    let db = setup_db("coco_test_init").await;

    let class = Class {
        name: "TestClass".to_string(),
        parents: None,
        static_properties: None,
        dynamic_properties: None,
    };
    db.create_class(&class).await.unwrap();

    let kb = create_kb();
    let _coco = CoCo::new(db.clone(), kb.clone()).await;

    let kb_guard = kb.lock().unwrap();
    assert!(kb_guard.get_class("TestClass").is_some());

    db.drop_db().await.unwrap();
}

#[tokio::test]
async fn test_create_class_propagation() {
    let db = setup_db("coco_test_prop").await;
    let kb = create_kb();
    let coco = CoCo::new(db.clone(), kb.clone()).await;

    let class = Class {
        name: "NewClass".to_string(),
        parents: None,
        static_properties: None,
        dynamic_properties: None,
    };

    coco.create_class(class).await.unwrap();

    let classes = db.get_classes().await.unwrap();
    assert!(classes.iter().any(|c| c.name == "NewClass"));
    assert!(kb.lock().unwrap().get_class("NewClass").is_some());

    db.drop_db().await.unwrap();
}

#[tokio::test]
async fn test_create_object_and_add_class_event() {
    let db = setup_db("coco_test_event").await;
    let kb = create_kb();
    let coco = CoCo::new(db.clone(), kb.clone()).await;

    // 1. Create Class
    let mut static_props = HashMap::new();
    static_props.insert("age".to_string(), Property::Int { nullable: Some(false), default: Some(0), min: None, max: None });

    let person_class = Class {
        name: "Person".to_string(),
        parents: None,
        static_properties: Some(static_props),
        dynamic_properties: None,
    };
    coco.create_class(person_class).await.unwrap();

    let adult_class = Class {
        name: "Adult".to_string(),
        parents: None,
        static_properties: None,
        dynamic_properties: None,
    };
    coco.create_class(adult_class).await.unwrap();

    // 2. Create Rule: If Person age > 18 => add-class Adult
    let rule = Rule {
        name: "check-age".to_string(),
        content: "(defrule check-age (Person_age (id ?id) (value ?v&:(> ?v 18))) => (add-class ?id Adult))".to_string(),
    };
    coco.create_rule(rule).await.unwrap();

    // 3. Create Object
    let mut classes = HashSet::new();
    classes.insert("Person".to_string());

    let mut props = HashMap::new();
    props.insert("age".to_string(), Value::Int(20));

    let object = Object { id: None, classes, properties: Some(props), values: None };
    let p1_id = coco.create_object(object).await.unwrap();

    // 4. Run KB to trigger rule
    {
        let kb_guard = kb.lock().unwrap();
        kb_guard.run().unwrap();
    }

    // 5. Verify Event Propagation
    sleep(Duration::from_millis(500)).await;

    // Verify object in DB has updated class
    let objects = db.get_objects().await.unwrap();
    let p1 = objects.iter().find(|o| o.id.as_ref() == Some(&p1_id)).expect("p1 should exist");
    assert!(p1.classes.contains("Adult"), "DB object should have Adult class");

    db.drop_db().await.unwrap();
}

#[tokio::test]
async fn test_add_data_event() {
    let db = setup_db("coco_test_add_data").await;
    let kb = create_kb();
    let coco = CoCo::new(db.clone(), kb.clone()).await;

    // 1. Create Class with dynamic property
    let mut dynamic_props = HashMap::new();
    dynamic_props.insert("temp".to_string(), Property::Float { nullable: Some(false), default: Some(0.0), min: None, max: None });

    let sensor_class = Class {
        name: "Sensor".to_string(),
        parents: None,
        static_properties: None,
        dynamic_properties: Some(dynamic_props),
    };
    coco.create_class(sensor_class).await.unwrap();

    // 2. Create Rule: If temperature > 50 => add-data (log 0.0)
    let rule = Rule {
        name: "log-high-temp".to_string(),
        content: "(defrule log-high-temp (Sensor_temp (id ?id) (value ?v&:(> ?v 50.0))) => (add-data ?id (create$ temp) (create$ 0.0)))".to_string(),
    };
    coco.create_rule(rule).await.unwrap();

    // 3. Create Object
    let mut classes = HashSet::new();
    classes.insert("Sensor".to_string());
    let object = Object { id: None, classes, properties: None, values: Some(HashMap::new()) };
    let s1_id = coco.create_object(object).await.unwrap();

    // 4. Add data manually to trigger rule
    let mut values = HashMap::new();
    values.insert("temp".to_string(), Value::Float(100.0));

    coco.add_data(&s1_id, values, Utc::now()).await.unwrap();

    {
        let kb_guard = kb.lock().unwrap();
        kb_guard.run().unwrap();
    }

    sleep(Duration::from_millis(500)).await;

    // Verify data in DB
    // We expect:
    // 1. Initial manual add_data (100.0) - from direct call
    // 2. Loopback from KB manual add_data (100.0) - might result in redundant store or just same val
    // 3. Rule triggered add_data (0.0)

    let stored_values = db.get_values(&s1_id, &Utc::now().checked_sub_signed(chrono::Duration::minutes(1)).unwrap(), &Utc::now().checked_add_signed(chrono::Duration::minutes(1)).unwrap()).await.unwrap();

    let temp_history = stored_values.get("temp").expect("Should have temp values");

    // Check for presence of 100.0 and 0.0
    let has_100 = temp_history.iter().any(|(v, _)| matches!(v, Value::Float(f) if *f == 100.0));
    let has_0 = temp_history.iter().any(|(v, _)| matches!(v, Value::Float(f) if *f == 0.0));

    assert!(has_100, "Should have logged 100.0");
    assert!(has_0, "Should have logged 0.0 from rule");

    db.drop_db().await.unwrap();
}

use crate::db::{Class, Database, DynamicValue, Object, Property, Rule, StaticValue};
use rust_rule_engine::{
    Facts, GRLParser, KnowledgeBase, RustRuleEngine, Value,
    rete::{FactValue, FieldDef, FieldType, Template, TemplateRegistry},
};
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
    template_registry: TemplateRegistry,
    facts: Facts,
    engine: RustRuleEngine,
}

impl CoCo {
    pub async fn new(db: Box<dyn Database + Send + Sync>) -> Self {
        let name = &db.name().to_string();
        let objects = Arc::new(RwLock::new(HashMap::new()));
        let mut coco = Self {
            db,
            classes: HashMap::new(),
            objects: objects.clone(),
            rules: HashMap::new(),
            template_registry: TemplateRegistry::new(),
            facts: Facts::new(),
            engine: RustRuleEngine::new(KnowledgeBase::new(name)),
        };

        coco.engine.register_function("add_class", move |args, _| {
            let object_id = &args[0].to_string();
            let class_name = args[1].to_string();
            objects
                .read()
                .expect("Failed to lock objects map")
                .get(object_id)
                .expect("Object not found")
                .write()
                .expect("Failed to lock object")
                .classes
                .get_or_insert_with(HashSet::new)
                .insert(class_name);
            Ok(rust_rule_engine::Value::Null)
        });

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
            let mut template = Template::new(class.name.clone());
            template.add_field(FieldDef {
                name: "id".to_string(),
                field_type: FieldType::String,
                default_value: None,
                required: true,
            });
            for (_, prop) in class
                .static_properties
                .iter()
                .chain(class.dynamic_properties.iter())
                .flatten()
            {
                template.add_field(Self::to_field_def(prop));
            }
            for (_, prop) in class.dynamic_properties.iter().flatten() {
                template.add_field(Self::to_timestamp_field_def(prop));
            }
            self.template_registry.register(template);
            self.classes.insert(class.name.to_string(), class);
        }
    }

    fn to_field_def(prop: &Property) -> FieldDef {
        match prop {
            Property::Bool {
                name,
                required,
                default,
            } => FieldDef {
                name: name.clone(),
                field_type: FieldType::Boolean,
                default_value: default.map(|v| FactValue::Boolean(v)),
                required: required.unwrap_or(false),
            },
            Property::Int {
                name,
                required,
                default,
                ..
            } => FieldDef {
                name: name.clone(),
                field_type: FieldType::Integer,
                default_value: default.map(|v| FactValue::Integer(v)),
                required: required.unwrap_or(false),
            },
            Property::Float {
                name,
                required,
                default,
                ..
            } => FieldDef {
                name: name.clone(),
                field_type: FieldType::Float,
                default_value: default.map(|v| FactValue::Float(v)),
                required: required.unwrap_or(false),
            },
        }
    }

    fn to_timestamp_field_def(prop: &Property) -> FieldDef {
        match prop {
            Property::Bool { name, .. } => FieldDef {
                name: name.clone() + "_timestamp",
                field_type: FieldType::Integer,
                default_value: None,
                required: true,
            },
            Property::Int { name, .. } => FieldDef {
                name: name.clone() + "_timestamp",
                field_type: FieldType::Integer,
                default_value: None,
                required: true,
            },
            Property::Float { name, .. } => FieldDef {
                name: name.clone() + "_timestamp",
                field_type: FieldType::Integer,
                default_value: None,
                required: true,
            },
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

    fn to_static_value(property: &Property, value: &StaticValue) -> Result<Value, Box<dyn Error>> {
        match (property, value) {
            (Property::Bool { .. }, StaticValue::Bool(b)) => Ok(Value::Boolean(*b)),
            (Property::Int { min, max, .. }, StaticValue::Int(i)) => {
                if let Some(min_val) = min {
                    if *i < *min_val as i64 {
                        return Err(format!(
                            "Integer value {} is less than minimum {}",
                            i, min_val
                        )
                        .into());
                    }
                }
                if let Some(max_val) = max {
                    if *i > *max_val as i64 {
                        return Err(format!(
                            "Integer value {} is greater than maximum {}",
                            i, max_val
                        )
                        .into());
                    }
                }
                Ok(Value::Integer(*i))
            }
            (Property::Float { min, max, .. }, StaticValue::Float(f)) => {
                if let Some(min_val) = min {
                    if *f < *min_val {
                        return Err(
                            format!("Float value {} is less than minimum {}", f, min_val).into(),
                        );
                    }
                }
                if let Some(max_val) = max {
                    if *f > *max_val {
                        return Err(format!(
                            "Float value {} is greater than maximum {}",
                            f, max_val
                        )
                        .into());
                    }
                }
                Ok(Value::Number(*f))
            }
            _ => Err("Type mismatch between property and static value".into()),
        }
    }

    fn to_dynamic_value(
        property: &Property,
        dynamic_value: &DynamicValue,
    ) -> Result<(Value, Value), Box<dyn Error>> {
        match (property, dynamic_value) {
            (Property::Bool { .. }, DynamicValue::Bool(b, timestamp)) => {
                Ok((Value::Boolean(*b), Value::Integer(timestamp.timestamp())))
            }
            (Property::Int { min, max, .. }, DynamicValue::Int(i, timestamp)) => {
                if let Some(min_val) = min {
                    if *i < *min_val as i64 {
                        return Err(format!(
                            "Integer value {} is less than minimum {}",
                            i, min_val
                        )
                        .into());
                    }
                }
                if let Some(max_val) = max {
                    if *i > *max_val as i64 {
                        return Err(format!(
                            "Integer value {} is greater than maximum {}",
                            i, max_val
                        )
                        .into());
                    }
                }
                Ok((Value::Integer(*i), Value::Integer(timestamp.timestamp())))
            }
            (Property::Float { min, max, .. }, DynamicValue::Float(f, timestamp)) => {
                if let Some(min_val) = min {
                    if *f < *min_val {
                        return Err(
                            format!("Float value {} is less than minimum {}", f, min_val).into(),
                        );
                    }
                }
                if let Some(max_val) = max {
                    if *f > *max_val {
                        return Err(format!(
                            "Float value {} is greater than maximum {}",
                            f, max_val
                        )
                        .into());
                    }
                }
                Ok((Value::Number(*f), Value::Integer(timestamp.timestamp())))
            }
            _ => Err("Type mismatch between property and dynamic value".into()),
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
            self.engine
                .knowledge_base()
                .add_rule(
                    GRLParser::parse_rule(rule.content.as_str()).expect("Failed to parse rule"),
                )
                .expect("Failed to add rule to knowledge base");
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

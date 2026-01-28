use crate::db::{Class, Database, Object, Property, Rule};
use rust_rule_engine::{
    Facts, GRLParser, KnowledgeBase, RustRuleEngine,
    rete::{FactValue, FieldDef, FieldType, Template, TemplateRegistry},
};
use std::{
    collections::HashMap,
    error::Error,
    sync::{Arc, RwLock},
};

pub struct CoCo {
    db: Box<dyn Database + Send + Sync>,
    classes: HashMap<String, Class>,
    objects: HashMap<String, Object>,
    rules: HashMap<String, Rule>,
    template_registry: TemplateRegistry,
    facts: Facts,
    engine: RustRuleEngine,
}

impl CoCo {
    pub async fn new(db: Box<dyn Database + Send + Sync>) -> Self {
        let name = &db.name().to_string();
        let mut coco = Self {
            db,
            classes: HashMap::new(),
            objects: HashMap::new(),
            rules: HashMap::new(),
            template_registry: TemplateRegistry::new(),
            facts: Facts::new(),
            engine: RustRuleEngine::new(KnowledgeBase::new(name)),
        };

        coco.engine.register_function("add_class", move |args, _| {
            // objects
            //     .read()
            //     .unwrap()
            //     .get(&args[0].as_string().unwrap())
            //     .expect("Object not found")
            //     .classes
            //     .insert(args[1].as_string().unwrap());
            Ok(rust_rule_engine::Value::Null)
        });

        coco.add_classes(coco.db.get_classes().await.unwrap());
        coco.add_rules(coco.db.get_rules().await.unwrap());

        coco
    }

    pub fn get_class(&self, name: &str) -> Option<&Class> {
        self.classes.get(name)
    }

    pub async fn create_class(
        &mut self,
        name: &str,
        static_properties: HashMap<String, Property>,
        dynamic_properties: HashMap<String, Property>,
    ) {
        let class = Class {
            name: name.to_string(),
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
            for (_, prop) in class.static_properties.iter() {
                let field_def = match prop {
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
                };
                template.add_field(field_def);
            }
            self.template_registry.register(template);
            self.classes.insert(class.name.to_string(), class);
        }
    }

    pub fn get_object(&self, id: &str) -> Option<&Object> {
        self.objects.get(id)
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

    pub(crate) fn add_rules(&mut self, db_rules: Vec<Rule>) {
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

    #[tokio::test]
    async fn test_coco_initialization() {
        let coco = CoCo::new(Box::new(
            MongoDB::new("coco_test", "mongodb://localhost:27017")
                .await
                .unwrap(),
        ))
        .await;

        coco.drop_db().await.unwrap();
    }
}

use crate::{
    class::Class,
    db::{DBClass, DBRule, Database},
    object::Object,
    property::{BoolPropertyType, FloatPropertyType, IntPropertyType, PropertyType},
    rule::Rule,
};
use rust_rule_engine::{Facts, GRLParser, KnowledgeBase, RustRuleEngine};
use std::{
    cell::RefCell,
    collections::HashMap,
    error::Error,
    rc::{Rc, Weak},
};

pub struct CoCo {
    weak_self: Weak<Self>,
    db: Box<dyn Database>,
    property_types: RefCell<HashMap<String, Rc<dyn PropertyType>>>,
    classes: RefCell<HashMap<String, Rc<Class>>>,
    objects: RefCell<HashMap<String, Rc<Object>>>,
    rules: RefCell<HashMap<String, Rc<Rule>>>,
    facts: Facts,
    engine: RustRuleEngine,
}

impl CoCo {
    pub async fn new(db: Box<dyn Database>) -> Rc<Self> {
        let name = &db.name().to_string();
        let coco = Rc::new_cyclic(|weak_self| Self {
            weak_self: weak_self.clone(),
            db,
            property_types: RefCell::new(HashMap::new()),
            classes: RefCell::new(HashMap::new()),
            objects: RefCell::new(HashMap::new()),
            rules: RefCell::new(HashMap::new()),
            facts: Facts::new(),
            engine: RustRuleEngine::new(KnowledgeBase::new(name)),
        });

        coco.add_property_type(Rc::new(BoolPropertyType::new(coco.weak_self.clone())));
        coco.add_property_type(Rc::new(IntPropertyType::new(coco.weak_self.clone())));
        coco.add_property_type(Rc::new(FloatPropertyType::new(coco.weak_self.clone())));

        coco.add_classes(coco.db.get_classes().await.unwrap());
        coco.add_rules(coco.db.get_rules().await.unwrap());

        coco
    }

    pub fn get_property_type(&self, name: &str) -> Option<Rc<dyn PropertyType>> {
        self.property_types.borrow().get(name).cloned()
    }

    pub fn add_property_type(&self, property_type: Rc<dyn PropertyType>) {
        self.property_types
            .borrow_mut()
            .insert(property_type.name().to_string(), property_type);
    }

    pub fn get_class(&self, name: &str) -> Option<Rc<Class>> {
        self.classes.borrow().get(name).cloned()
    }

    fn add_classes(&self, db_classes: Vec<DBClass>) {
        for db_class in &db_classes {
            let class = Rc::new(Class::from_db_class(self.weak_self.clone(), db_class));
            self.classes
                .borrow_mut()
                .insert(class.name().to_string(), class.into());
        }
    }

    pub fn get_object(&self, id: &str) -> Option<Rc<Object>> {
        self.objects.borrow().get(id).cloned()
    }

    pub fn get_rule(&self, name: &str) -> Option<Rc<Rule>> {
        self.rules.borrow().get(name).cloned()
    }

    pub async fn create_rule(&self, name: &str, content: &str) {
        let rule = DBRule {
            name: name.to_string(),
            content: content.to_string(),
        };
        self.db
            .create_rule(&rule)
            .await
            .expect("Failed to create rule in database");
        self.add_rules(vec![rule]);
    }

    pub(crate) fn add_rules(&self, db_rules: Vec<DBRule>) {
        for db_rule in db_rules {
            self.engine
                .knowledge_base()
                .add_rule(
                    GRLParser::parse_rule(db_rule.content.as_str()).expect("Failed to parse rule"),
                )
                .expect("Failed to add rule to knowledge base");
            let rule = Rc::new(Rule::from_db_rule(db_rule));
            self.rules
                .borrow_mut()
                .insert(rule.name().to_string(), rule.clone());
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
        assert!(coco.get_property_type("bool").is_some());
        assert!(coco.get_property_type("int").is_some());
        assert!(coco.get_property_type("float").is_some());

        coco.drop_db().await.unwrap();
    }
}

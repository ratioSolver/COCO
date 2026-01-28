use crate::{
    class::Class,
    db::{DBRule, Database},
    object::Object,
    property::{BoolPropertyType, FloatPropertyType, IntPropertyType, PropertyType},
    rule::Rule,
};
use rust_rule_engine::{KnowledgeBase, RustRuleEngine};
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
    kb: KnowledgeBase,
    engine: RustRuleEngine,
}

impl CoCo {
    pub async fn new(db: Box<dyn Database>) -> Rc<Self> {
        let name = &db.name().to_string();
        let coco = Rc::new_cyclic(|weak_self| {
            let kb = KnowledgeBase::new(name);
            Self {
                weak_self: weak_self.clone(),
                db,
                property_types: RefCell::new(HashMap::new()),
                classes: RefCell::new(HashMap::new()),
                objects: RefCell::new(HashMap::new()),
                rules: RefCell::new(HashMap::new()),
                kb: kb.clone(),
                engine: RustRuleEngine::new(kb),
            }
        });

        coco.add_property_type(Rc::new(BoolPropertyType::new(coco.weak_self.clone())));
        coco.add_property_type(Rc::new(IntPropertyType::new(coco.weak_self.clone())));
        coco.add_property_type(Rc::new(FloatPropertyType::new(coco.weak_self.clone())));

        coco.add_classes(
            coco.db
                .get_classes()
                .await
                .unwrap()
                .into_iter()
                .map(|db_class| Rc::new(Class::from_db_class(coco.weak_self.clone(), db_class)))
                .collect(),
        );

        coco.add_rules(
            coco.db
                .get_rules()
                .await
                .unwrap()
                .into_iter()
                .map(|db_rule| Rc::new(Rule::from_db_rule(db_rule)))
                .collect(),
        );

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

    fn add_classes(&self, classes: Vec<Rc<Class>>) {
        for class in classes {
            self.classes
                .borrow_mut()
                .insert(class.name().to_string(), class);
        }
    }

    pub fn get_object(&self, id: &str) -> Option<Rc<Object>> {
        self.objects.borrow().get(id).cloned()
    }

    pub fn get_rule(&self, name: &str) -> Option<Rc<Rule>> {
        self.rules.borrow().get(name).cloned()
    }

    pub async fn create_rule(&self, name: &str, content: &str) -> Rc<Rule> {
        let rule = DBRule {
            name: name.to_string(),
            content: content.to_string(),
        };
        match self.db.create_rule(&rule).await {
            Ok(_) => (),
            Err(e) => {
                panic!("Failed to create rule in database: {}", e);
            }
        }
        let rule = Rc::new(Rule::from_db_rule(rule));
        self.add_rules(vec![rule.clone()]);
        rule
    }

    fn add_rules(&self, rules: Vec<Rc<Rule>>) {
        let rules_grl: String = rules
            .iter()
            .map(|r| r.content().to_string())
            .collect::<Vec<String>>()
            .join("\n");
        match self.kb.add_rules_from_grl(rules_grl.as_str()) {
            Ok(_) => (),
            Err(e) => {
                panic!("Failed to add rule to knowledge base: {}", e);
            }
        }
        for rule in rules {
            self.rules
                .borrow_mut()
                .insert(rule.name().to_string(), rule);
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

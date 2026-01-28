use crate::{
    class::Class,
    db::{DBClass, DBRule, Database},
    object::Object,
    rule::Rule,
};
use rust_rule_engine::{Facts, GRLParser, KnowledgeBase, RustRuleEngine};
use std::{
    collections::HashMap,
    error::Error,
    rc::{Rc, Weak},
};

pub struct CoCo {
    weak_self: Weak<Self>,
    db: Box<dyn Database>,
    classes: HashMap<String, Rc<Class>>,
    objects: HashMap<String, Rc<Object>>,
    rules: HashMap<String, Rc<Rule>>,
    facts: Facts,
    engine: RustRuleEngine,
}

impl CoCo {
    pub async fn new(db: Box<dyn Database>) -> Self {
        let name = &db.name().to_string();
        let mut coco = Self {
            weak_self: Weak::new(),
            db,
            classes: HashMap::new(),
            objects: HashMap::new(),
            rules: HashMap::new(),
            facts: Facts::new(),
            engine: RustRuleEngine::new(KnowledgeBase::new(name)),
        };

        coco.add_classes(coco.db.get_classes().await.unwrap());
        coco.add_rules(coco.db.get_rules().await.unwrap());

        coco
    }

    pub fn get_class(&self, name: &str) -> Option<Rc<Class>> {
        self.classes.get(name).cloned()
    }

    fn add_classes(&mut self, db_classes: Vec<DBClass>) {
        for db_class in db_classes {
            let class = Rc::new(Class::new(self.weak_self.clone(), db_class));
            self.classes.insert(class.name().to_string(), class.into());
        }
    }

    pub fn get_object(&self, id: &str) -> Option<Rc<Object>> {
        self.objects.get(id).cloned()
    }

    pub fn get_rule(&self, name: &str) -> Option<Rc<Rule>> {
        self.rules.get(name).cloned()
    }

    pub async fn create_rule(&mut self, name: &str, content: &str) {
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

    pub(crate) fn add_rules(&mut self, db_rules: Vec<DBRule>) {
        for db_rule in db_rules {
            self.engine
                .knowledge_base()
                .add_rule(
                    GRLParser::parse_rule(db_rule.content.as_str()).expect("Failed to parse rule"),
                )
                .expect("Failed to add rule to knowledge base");
            let rule = Rc::new(Rule::new(db_rule));
            self.rules.insert(rule.name().to_string(), rule.clone());
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

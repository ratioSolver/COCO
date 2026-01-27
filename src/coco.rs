use crate::{
    db::Database,
    item::Item,
    kind::Kind,
    property::{BoolPropertyType, FloatPropertyType, IntPropertyType, PropertyType},
    rule::Rule,
};
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
    kinds: RefCell<HashMap<String, Rc<Kind>>>,
    items: RefCell<HashMap<String, Rc<Item>>>,
    rules: RefCell<HashMap<String, Rc<Rule>>>,
}

impl CoCo {
    pub async fn new(db: Box<dyn Database>) -> Rc<Self> {
        let coco = Rc::new_cyclic(|weak_self| Self {
            weak_self: weak_self.clone(),
            db,
            property_types: RefCell::new(HashMap::new()),
            kinds: RefCell::new(HashMap::new()),
            items: RefCell::new(HashMap::new()),
            rules: RefCell::new(HashMap::new()),
        });

        coco.add_property_type(Rc::new(BoolPropertyType::new(coco.weak_self.clone())));
        coco.add_property_type(Rc::new(IntPropertyType::new(coco.weak_self.clone())));
        coco.add_property_type(Rc::new(FloatPropertyType::new(coco.weak_self.clone())));

        for kind in coco.db.get_types().await.unwrap() {
            coco.add_kind(Rc::new(Kind::from_db_kind(coco.weak_self.clone(), kind)));
        }

        for rule in coco.db.get_rules().await.unwrap() {
            coco.add_rule(Rc::new(Rule::from_db_rule(rule)));
        }

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

    pub fn get_kind(&self, name: &str) -> Option<Rc<Kind>> {
        self.kinds.borrow().get(name).cloned()
    }

    pub fn add_kind(&self, kind: Rc<Kind>) {
        self.kinds
            .borrow_mut()
            .insert(kind.name().to_string(), kind);
    }

    pub fn get_item(&self, id: &str) -> Option<Rc<Item>> {
        self.items.borrow().get(id).cloned()
    }

    pub fn get_rule(&self, name: &str) -> Option<Rc<Rule>> {
        self.rules.borrow().get(name).cloned()
    }

    pub fn add_rule(&self, rule: Rc<Rule>) {
        self.rules
            .borrow_mut()
            .insert(rule.name().to_string(), rule);
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

use crate::{db::Database, item::Item, kind::Kind, property::PropertyType, rule::Rule};
use std::{collections::HashMap, rc::Rc};

pub struct CoCo {
    db: Database,
    property_types: HashMap<String, Rc<PropertyType>>,
    kinds: HashMap<String, Rc<Kind>>,
    items: HashMap<String, Rc<Item>>,
    rules: HashMap<String, Rc<Rule>>,
}

impl CoCo {
    pub async fn new() -> Self {
        Self {
            db: Database::new("mongodb://localhost:27017").await.unwrap(),
            property_types: HashMap::new(),
            kinds: HashMap::new(),
            items: HashMap::new(),
            rules: HashMap::new(),
        }
    }

    pub fn get_property_type(&self, name: &str) -> Option<Rc<PropertyType>> {
        self.property_types.get(name).cloned()
    }

    pub fn get_kind(&self, name: &str) -> Option<Rc<Kind>> {
        self.kinds.get(name).cloned()
    }

    pub fn get_item(&self, id: &str) -> Option<Rc<Item>> {
        self.items.get(id).cloned()
    }

    pub fn get_rule(&self, name: &str) -> Option<Rc<Rule>> {
        self.rules.get(name).cloned()
    }
}

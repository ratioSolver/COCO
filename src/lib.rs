mod item;
mod kind;
mod property;
mod rule;

use rust_rule_engine::Rule;

use crate::{kind::Kind, property::PropertyType};
use std::{collections::HashMap, rc::Rc};

pub struct CoCo {
    property_types: HashMap<String, Rc<PropertyType>>,
    kinds: HashMap<String, Rc<Kind>>,
    rules: HashMap<String, Rc<Rule>>,
}

impl CoCo {
    pub fn new() -> Self {
        let coco = Self {
            property_types: HashMap::new(),
            kinds: HashMap::new(),
            rules: HashMap::new(),
        };
        coco
    }

    pub fn get_property_type(&self, name: &str) -> Option<Rc<PropertyType>> {
        self.property_types.get(name).cloned()
    }

    pub fn get_kind(&self, name: &str) -> Option<Rc<Kind>> {
        self.kinds.get(name).cloned()
    }

    pub fn get_rule(&self, name: &str) -> Option<Rc<Rule>> {
        self.rules.get(name).cloned()
    }
}

use crate::{CoCo, Property, db::DBClass, object::Object};
use std::{
    collections::HashMap,
    sync::{Arc, Weak},
};

pub struct Class {
    coco: Weak<CoCo>,
    name: String,
    static_properties: HashMap<String, Property>,
    dynamic_properties: HashMap<String, Property>,
    instances: Vec<Arc<Object>>,
}

impl Class {
    pub(super) fn new(coco: Weak<CoCo>, db_class: DBClass) -> Self {
        Self {
            coco,
            name: db_class.name,
            static_properties: db_class.static_properties,
            dynamic_properties: db_class.dynamic_properties,
            instances: Vec::new(),
        }
    }

    pub fn coco(&self) -> &Weak<CoCo> {
        &self.coco
    }

    pub fn name(&self) -> &str {
        &self.name
    }

    pub fn static_properties(&self) -> &HashMap<String, Property> {
        &self.static_properties
    }

    pub fn dynamic_properties(&self) -> &HashMap<String, Property> {
        &self.dynamic_properties
    }
}

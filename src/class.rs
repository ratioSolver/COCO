use crate::{CoCo, Property, db::DBClass, object::Object};
use std::{
    collections::HashMap,
    rc::{Rc, Weak},
};

pub struct Class {
    coco: Weak<CoCo>,
    name: String,
    static_properties: HashMap<String, Property>,
    dynamic_properties: HashMap<String, Property>,
    instances: Vec<Rc<Object>>,
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
}

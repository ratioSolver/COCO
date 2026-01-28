use crate::{CoCo, db::DBClass, object::Object};
use std::rc::{Rc, Weak};

pub struct Class {
    coco: Weak<CoCo>,
    name: String,
    instances: Vec<Rc<Object>>,
}

impl Class {
    pub fn new(coco: Weak<CoCo>, name: &str) -> Self {
        Self {
            coco,
            name: name.to_string(),
            instances: Vec::new(),
        }
    }

    pub(super) fn from_db_class(coco: Weak<CoCo>, db_class: &DBClass) -> Self {
        Self {
            coco,
            name: db_class.name.clone(),
            instances: Vec::new(),
        }
    }

    pub(super) fn refine_from_db_class(&mut self, db_class: DBClass) {}

    pub fn coco(&self) -> &Weak<CoCo> {
        &self.coco
    }

    pub fn name(&self) -> &str {
        &self.name
    }
}

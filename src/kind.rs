use std::rc::{Rc, Weak};

use crate::{CoCo, item::Item};

pub struct Kind {
    coco: Weak<CoCo>,
    name: String,
    instances: Vec<Rc<Item>>,
}

impl Kind {
    pub fn new(coco: Weak<CoCo>, name: &str) -> Self {
        Self {
            coco,
            name: name.to_string(),
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

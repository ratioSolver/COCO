use crate::{CoCo, db::DBKind, item::Item};
use std::rc::{Rc, Weak};

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

    pub fn from_db_kind(coco: Weak<CoCo>, db_kind: DBKind) -> Self {
        Self {
            coco,
            name: db_kind.name,
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

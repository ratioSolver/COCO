use crate::{CoCo, db::DBItem, kind::Kind};
use std::rc::{Rc, Weak};

pub struct Item {
    kinds: Vec<Weak<Kind>>,
    id: String,
}

impl Item {
    pub fn new(kind: Vec<Weak<Kind>>, id: String) -> Self {
        Self { kinds: kind, id }
    }

    pub fn from_db_item(kind: Weak<CoCo>, db_item: DBItem) -> Self {
        Self {
            kinds: db_item
                .kinds
                .into_iter()
                .map(|kind_name| {
                    kind.upgrade()
                        .unwrap()
                        .get_kind(&kind_name)
                        .map(|k| Rc::downgrade(&k))
                        .unwrap()
                })
                .collect(),
            id: db_item.id,
        }
    }

    pub fn kinds(&self) -> &Vec<Weak<Kind>> {
        &self.kinds
    }

    pub fn id(&self) -> &str {
        &self.id
    }
}

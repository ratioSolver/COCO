use std::rc::Weak;

use crate::kind::Kind;

pub struct Item {
    id: String,
    kind: Weak<Kind>,
}

impl Item {
    pub fn new(id: String, kind: Weak<Kind>) -> Self {
        Self { id, kind }
    }

    pub fn kind(&self) -> &Weak<Kind> {
        &self.kind
    }
}

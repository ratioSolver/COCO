use std::rc::Weak;

use crate::kind::Kind;

pub struct Item {
    kind: Weak<Kind>,
}

impl Item {
    pub fn new(kind: Weak<Kind>) -> Self {
        Self { kind }
    }

    pub fn kind(&self) -> &Weak<Kind> {
        &self.kind
    }
}

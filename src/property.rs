use std::rc::Weak;

use crate::CoCo;

pub trait PropertyType {
    fn coco(&self) -> &Weak<CoCo>;
    fn name(&self) -> &str;
}

pub struct BoolPropertyType {
    coco: Weak<CoCo>,
}

impl BoolPropertyType {
    pub fn new(coco: Weak<CoCo>) -> Self {
        Self { coco }
    }
}

impl PropertyType for BoolPropertyType {
    fn coco(&self) -> &Weak<CoCo> {
        &self.coco
    }

    fn name(&self) -> &str {
        "bool"
    }
}

pub trait Property {
    fn kind(&self) -> &Weak<dyn PropertyType>;
    fn name(&self) -> &str;
}

pub struct BoolProperty {
    kind: Weak<dyn PropertyType>,
    name: String,
}

impl BoolProperty {
    pub fn new(kind: Weak<dyn PropertyType>, name: String) -> Self {
        Self { kind, name }
    }
}

impl Property for BoolProperty {
    fn kind(&self) -> &Weak<dyn PropertyType> {
        &self.kind
    }

    fn name(&self) -> &str {
        &self.name
    }
}

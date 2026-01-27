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

pub struct IntPropertyType {
    coco: Weak<CoCo>,
}

impl IntPropertyType {
    pub fn new(coco: Weak<CoCo>) -> Self {
        Self { coco }
    }
}

impl PropertyType for IntPropertyType {
    fn coco(&self) -> &Weak<CoCo> {
        &self.coco
    }

    fn name(&self) -> &str {
        "int"
    }
}

pub struct FloatPropertyType {
    coco: Weak<CoCo>,
}

impl FloatPropertyType {
    pub fn new(coco: Weak<CoCo>) -> Self {
        Self { coco }
    }
}

impl PropertyType for FloatPropertyType {
    fn coco(&self) -> &Weak<CoCo> {
        &self.coco
    }

    fn name(&self) -> &str {
        "float"
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

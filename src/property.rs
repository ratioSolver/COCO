use std::rc::Weak;

pub struct PropertyType {
    name: String,
}

impl PropertyType {
    pub fn new(name: &str) -> Self {
        Self {
            name: name.to_string(),
        }
    }

    pub fn name(&self) -> &str {
        &self.name
    }
}

pub struct Property {
    kind: Weak<PropertyType>,
    name: String,
}

impl Property {
    pub fn new(kind: Weak<PropertyType>, name: &str) -> Self {
        Self {
            kind,
            name: name.to_string(),
        }
    }

    pub fn kind(&self) -> &Weak<PropertyType> {
        &self.kind
    }

    pub fn name(&self) -> &str {
        &self.name
    }
}

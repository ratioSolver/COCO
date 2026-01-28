use crate::{CoCo, class::Class, db::DBObject};
use std::rc::{Rc, Weak};

pub struct Object {
    classes: Vec<Weak<Class>>,
    id: String,
}

impl Object {
    pub fn new(class: Vec<Weak<Class>>, id: String) -> Self {
        Self { classes: class, id }
    }

    pub fn from_db_object(class: Weak<CoCo>, db_object: DBObject) -> Self {
        Self {
            classes: db_object
                .classes
                .into_iter()
                .map(|class_name| {
                    class
                        .upgrade()
                        .unwrap()
                        .get_class(&class_name)
                        .map(|k| Rc::downgrade(&k))
                        .unwrap()
                })
                .collect(),
            id: db_object.id,
        }
    }

    pub fn classes(&self) -> &Vec<Weak<Class>> {
        &self.classes
    }

    pub fn id(&self) -> &str {
        &self.id
    }
}

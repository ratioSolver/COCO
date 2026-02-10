use crate::db::Database;

pub struct MongoDB {}

impl MongoDB {
    pub fn new() -> Self {
        Self {}
    }
}

impl Database for MongoDB {}

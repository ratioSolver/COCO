use crate::{db::Database, kb::KnowledgeBase};

pub mod db;
pub mod kb;
pub mod model;

pub struct CoCo<KB: KnowledgeBase, DB: Database> {
    knowledge_base: KB,
    database: DB,
}

impl<KB: KnowledgeBase, DB: Database> CoCo<KB, DB> {
    pub async fn new(database: DB, knowledge_base: KB) -> Self {
        CoCo { knowledge_base, database }
    }
}

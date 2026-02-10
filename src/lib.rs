use crate::{db::Database, kb::KnowledgeBase};

pub mod db;
pub mod kb;
pub mod model;

pub struct CoCo<KB: KnowledgeBase, DB: Database> {
    knowledge_base: KB,
    database: DB,
}

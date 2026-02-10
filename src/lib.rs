use crate::{db::Database, kb::KnowledgeBase};

mod db;
mod kb;
mod model;

pub struct CoCo<KB: KnowledgeBase, DB: Database> {
    knowledge_base: KB,
    database: DB,
}

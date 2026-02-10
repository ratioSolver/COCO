mod model;

pub trait KnowledgeBase {}

pub trait Database {}

pub struct CoCo<KB: KnowledgeBase, DB: Database> {
    knowledge_base: KB,
    database: DB,
}

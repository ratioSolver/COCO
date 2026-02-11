use crate::{db::Database, kb::KnowledgeBase, model::Class};

pub mod db;
pub mod kb;
pub mod model;

pub struct CoCo<KB: KnowledgeBase, DB: Database> {
    knowledge_base: KB,
    database: DB,
}

impl<KB: KnowledgeBase, DB: Database> CoCo<KB, DB> {
    pub async fn new(database: DB, knowledge_base: KB) -> Self {
        let mut coco = CoCo { knowledge_base, database };
        coco.add_classes(coco.database.get_classes().await.unwrap_or_else(|e| {
            eprintln!("Error fetching classes from database: {:?}", e);
            vec![]
        }));
        coco
    }

    fn add_classes(&mut self, classes: Vec<Class>) {
        for class in classes {
            self.knowledge_base.create_class(&class).unwrap_or_else(|e| {
                eprintln!("Error adding class {}: {:?}", class.name, e);
            });
        }
    }
}

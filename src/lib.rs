mod clips;
mod coco;
mod db;
mod kb;
mod mongo;

pub use clips::clips::KnowledgeBase as CLIPSKnowledgeBase;
pub use coco::CoCo;
pub use db::*;
pub use kb::*;
pub use mongo::db::Database as MongoDatabase;

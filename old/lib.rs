mod clips;
mod coco;
mod db;
mod kb;
mod mongo;

pub use clips::kb::KnowledgeBase as CLIPSKnowledgeBase;
pub use coco::CoCo;
pub use coco::Notifier;
pub use db::*;
pub use kb::*;
pub use mongo::db::Database as MongoDatabase;

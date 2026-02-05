use coco::{CLIPSKnowledgeBase, CoCo, MongoDBDataStore};
use std::sync::{Arc, Mutex};

#[tokio::main]
async fn main() {
    let db = Arc::new(MongoDBDataStore::new("coco_server", "mongodb://localhost:27017").await.unwrap());
    let kb = Arc::new(Mutex::new(CLIPSKnowledgeBase::new()));
    let coco = CoCo::new(db, kb).await;
}

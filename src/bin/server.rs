use coco::{CLIPSKnowledgeBase, CoCo, MongoDBDataStore};
use std::sync::{Arc, Mutex};
use tokio::sync::broadcast;

#[tokio::main]
async fn main() {
    let db = Arc::new(MongoDBDataStore::new("coco_server", "mongodb://localhost:27017").await.unwrap());
    let (tx, _rx) = broadcast::channel(100);
    let kb = Arc::new(Mutex::new(CLIPSKnowledgeBase::new(tx)));
    let coco = CoCo::new(db, kb).await;
}

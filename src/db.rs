use mongodb::bson::doc;
use mongodb::error::Result;
use mongodb::options::IndexOptions;
use mongodb::{Client, IndexModel};

pub struct Database {
    client: Client,
}

impl Database {
    pub async fn new(connection_string: &str) -> Result<Self> {
        let client = Client::with_uri_str(connection_string).await?;
        let db = client.database("CoCo");
        let collection_names = db.list_collection_names().await?;
        if collection_names.is_empty() {
            let types_collection = db.collection::<mongodb::bson::Document>("types");
            let index = IndexModel::builder()
                .keys(doc! { "name": 1 })
                .options(IndexOptions::builder().unique(true).build())
                .build();
            types_collection.create_index(index).await?;
            let item_data_collection = db.collection::<mongodb::bson::Document>("item_data");
            let index = IndexModel::builder()
                .keys(doc! { "item_id": 1, "timestamp": 1 })
                .options(IndexOptions::builder().unique(true).build())
                .build();
            item_data_collection.create_index(index).await?;
        }
        Ok(Self { client })
    }

    pub fn get_client(&self) -> &Client {
        &self.client
    }
}

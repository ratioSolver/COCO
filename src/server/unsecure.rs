use crate::{CoCo, db::Database, kb::KnowledgeBase};
use axum::{extract::State, response::IntoResponse};
use tracing::trace;

async fn get_classes<DB, KB>(State(state): State<CoCo<DB, KB>>) -> impl IntoResponse
where
    DB: Database,
    KB: KnowledgeBase,
{
    trace!("Handling request to list all classes");
}

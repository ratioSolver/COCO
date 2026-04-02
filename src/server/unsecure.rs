use crate::CoCo;
use axum::{Json, Router, extract::State, http::StatusCode, response::IntoResponse, routing::get};
use tracing::trace;

pub fn unsecure_coco_router(coco: CoCo) -> Router {
    Router::new().route("/classes", get(get_classes)).with_state(coco)
}

async fn get_classes(State(state): State<CoCo>) -> impl IntoResponse {
    trace!("Handling request to list all classes");
    match state.get_classes().await {
        Ok(classes) => Json(classes).into_response(),
        Err(e) => (StatusCode::INTERNAL_SERVER_ERROR, format!("Failed to get classes: {}", e)).into_response(),
    }
}

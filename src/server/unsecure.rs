use crate::CoCo;
use axum::{Json, extract::State, http::StatusCode, response::IntoResponse};
use tracing::trace;

async fn get_classes(State(state): State<CoCo>) -> impl IntoResponse {
    trace!("Handling request to list all classes");
    match state.get_classes().await {
        Ok(classes) => Json(classes).into_response(),
        Err(e) => (StatusCode::INTERNAL_SERVER_ERROR, format!("Failed to get classes: {}", e)).into_response(),
    }
}

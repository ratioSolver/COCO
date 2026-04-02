use std::{
    collections::{HashMap, HashSet},
    sync::Arc,
};

use crate::{
    CoCo,
    db::mongodb::MongoDB,
    kb::{KnowledgeBaseError, clips::CLIPSKnowledgeBase},
};
use axum::{
    Json, Router,
    extract::{Path, State},
    response::IntoResponse,
    routing::post,
};
use clips::{ClipsValue, Type};
use reqwest::{Client, StatusCode};
use tokio::sync::RwLock;
use tracing::trace;
use yup_oauth2::{ServiceAccountAuthenticator, read_service_account_key};

#[derive(Clone)]
pub struct FcmTokens {
    tokens: Arc<RwLock<HashMap<String, HashSet<String>>>>,
}

pub fn setup_fcm(db: &MongoDB, kb: &CLIPSKnowledgeBase) -> Result<Router, KnowledgeBaseError> {
    let project_id = std::env::var("FCM_PROJECT_ID").unwrap_or_else(|_| {
        panic!("Missing FCM_PROJECT_ID environment variable");
    });
    add_fcm(db, kb, project_id)
}

pub fn add_fcm(db: &MongoDB, kb: &CLIPSKnowledgeBase, project_id: String) -> Result<Router, KnowledgeBaseError> {
    let url = format!("https://fcm.googleapis.com/v1/projects/{}/messages:send", project_id);
    let client = Client::new();

    kb.add_udf(
        "send-message",
        None,
        3,
        3,
        vec![Type(Type::SYMBOL), Type(Type::STRING), Type(Type::STRING)],
        Box::new(move |_env, ctx| {
            let object_id = ctx.get_next_argument(Type(Type::SYMBOL)).expect("Failed to get object ID argument for send-message UDF");
            let object_id = if let ClipsValue::Symbol(s) = object_id { s } else { panic!("Expected symbol for object ID argument in send-message UDF") };
            let title = ctx.get_next_argument(Type(Type::STRING)).expect("Failed to get title argument for send-message UDF");
            let title = if let ClipsValue::String(s) = title { s } else { panic!("Expected string for title argument in send-message UDF") };
            let message = ctx.get_next_argument(Type(Type::STRING)).expect("Failed to get message argument for send-message UDF");
            let message = if let ClipsValue::String(s) = message { s } else { panic!("Expected string for message argument in send-message UDF") };
            ClipsValue::Void()
        }),
    )?;

    Ok(Router::new().route("/add_token/{id}", post(add_token)).with_state(FcmTokens { tokens: Arc::new(RwLock::new(HashMap::new())) }))
}

async fn get_token() -> String {
    let key = read_service_account_key("service-account.json").await.expect("Failed to read service account key");
    let auth = ServiceAccountAuthenticator::builder(key).build().await.expect("Failed to create authenticator");
    let scopes = &["https://www.googleapis.com/auth/firebase.messaging"];
    let token = auth.token(scopes).await.expect("Failed to get token");
    token.token().unwrap().to_owned()
}

async fn add_token(State(state): State<FcmTokens>, Path(id): Path<String>, token: String) -> impl IntoResponse {
    trace!("Adding FCM token for ID: {}", id);
    state.tokens.write().await.entry(id.clone()).or_insert_with(HashSet::new).insert(token);
    (StatusCode::OK, format!("Token added for ID: {}", id)).into_response()
}

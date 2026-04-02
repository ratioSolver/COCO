use crate::kb::{KnowledgeBaseError, clips::CLIPSKnowledgeBase};
use clips::{ClipsValue, Type};
use reqwest::Client;
use yup_oauth2::{ServiceAccountAuthenticator, read_service_account_key};

pub fn add_fcm(kb: &CLIPSKnowledgeBase, project_id: String) -> Result<(), KnowledgeBaseError> {
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
    Ok(())
}

async fn get_token() -> String {
    let key = read_service_account_key("service-account.json").await.expect("Failed to read service account key");
    let auth = ServiceAccountAuthenticator::builder(key).build().await.expect("Failed to create authenticator");
    let scopes = &["https://www.googleapis.com/auth/firebase.messaging"];
    let token = auth.token(scopes).await.expect("Failed to get token");
    token.token().unwrap().to_owned()
}

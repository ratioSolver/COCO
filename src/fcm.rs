use yup_oauth2::{ServiceAccountAuthenticator, read_service_account_key};

pub struct FCMClient {
    project_id: String,
    client: reqwest::Client,
}

impl FCMClient {
    pub fn new(project_id: String) -> Self {
        Self { project_id, client: reqwest::Client::new() }
    }

    pub async fn send_message(&self, tokens: Vec<String>, title: &str, message: &str) -> Result<Vec<String>, reqwest::Error> {
        let token = get_token().await;
        let url = format!("https://fcm.googleapis.com/v1/projects/{}/messages:send", self.project_id);
        let mut failed_tokens = Vec::new();
        for tkn in tokens {
            let payload = serde_json::json!({
                "message": {
                    "token": tkn,
                    "notification": {
                        "title": title,
                        "body": message
                    }
                }
            });
            let response = self.client.post(&url).bearer_auth(&token).json(&payload).send().await?;
            if !response.status().is_success() {
                failed_tokens.push(tkn);
            }
        }
        Ok(failed_tokens)
    }
}

async fn get_token() -> String {
    let key = read_service_account_key("service-account.json").await.expect("Failed to read service account key");
    let auth = ServiceAccountAuthenticator::builder(key).build().await.expect("Failed to create authenticator");
    let scopes = &["https://www.googleapis.com/auth/firebase.messaging"];
    let token = auth.token(scopes).await.expect("Failed to get token");
    token.token().unwrap().to_string()
}

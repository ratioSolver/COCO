use yup_oauth2::{ServiceAccountAuthenticator, read_service_account_key};

pub struct FcmClient {
    project_id: String,
    client: reqwest::Client,
}

impl FcmClient {
    pub fn new(project_id: String) -> Self {
        Self { project_id, client: reqwest::Client::new() }
    }

    pub async fn send_message(&self, title: &str, message: &str) -> Result<(), reqwest::Error> {
        let token = get_token().await;
        let url = format!("https://fcm.googleapis.com/v1/projects/{}/messages:send", self.project_id);
        let body = serde_json::json!({
            "message": {
                "token": token,
                "notification": {
                    "title": title,
                    "body": message,
                }
            }
        });

        let response = self.client.post(&url).bearer_auth(token).json(&body).send().await?;
        if !response.status().is_success() {
            eprintln!("Failed to send message: {}", response.text().await?);
        }
        Ok(())
    }
}

async fn get_token() -> String {
    let key = read_service_account_key("service-account.json").await.expect("Failed to read service account key");
    let auth = ServiceAccountAuthenticator::builder(key).build().await.expect("Failed to create authenticator");
    let scopes = &["https://www.googleapis.com/auth/firebase.messaging"];
    let token = auth.token(scopes).await.expect("Failed to get token");
    token.token().unwrap().to_string()
}

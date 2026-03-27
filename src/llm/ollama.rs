use crate::llm::{Callback, LLM, LLMError};
use async_trait::async_trait;
use reqwest::Client;

pub struct Ollama {
    host: String,
    port: u16,
    model: String,
    client: Client,
    callback: Option<Callback>,
}

impl Ollama {
    pub fn new(host: String, port: u16, model: String) -> Self {
        Ollama { host: host, port, model: model, client: Client::new(), callback: None }
    }
}

#[async_trait]
impl LLM for Ollama {
    async fn prompt(&self, prompt: &str) -> Result<String, LLMError> {
        let url = format!("http://{}:{}/api/chat", self.host, self.port);
        let body = serde_json::json!({
            "model": self.model,
            "messages": [
                {
                    "role": "user",
                    "content": prompt
                }
            ],
            "stream": false
        });

        let client = self.client.clone();
        let callback = self.callback.clone();

        tokio::spawn(async move {
            let response_content = match client.post(&url).json(&body).send().await {
                Ok(response) => match response.json::<serde_json::Value>().await {
                    Ok(json) => json["message"]["content"].as_str().map(|content| content.to_owned()),
                    Err(_) => None,
                },
                Err(_) => None,
            };

            if let (Some(cb), Some(content)) = (callback, response_content) {
                cb(content);
            }
        });

        Ok("request queued".to_owned())
    }

    fn set_callback(&mut self, cb: Callback) {
        self.callback.replace(cb);
    }
}

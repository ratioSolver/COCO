use crate::llm::{LLM, LLMError};
use async_trait::async_trait;
use reqwest::Client;

pub struct Ollama {
    host: String,
    port: u16,
    model: String,
    client: Client,
}

impl Ollama {
    pub fn new(host: &str, port: u16, model: &str) -> Self {
        Ollama { host: host.to_string(), port, model: model.to_string(), client: Client::new() }
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

        match self.client.post(&url).json(&body).send().await {
            Ok(response) => match response.json::<serde_json::Value>().await {
                Ok(json) => {
                    if let Some(content) = json["message"]["content"].as_str() {
                        Ok(content.to_string())
                    } else {
                        Err(LLMError::GenerationError("Invalid response format".into()))
                    }
                }
                Err(_) => Err(LLMError::GenerationError("Failed to parse response".into())),
            },
            Err(_) => Err(LLMError::ConnectionError("Failed to connect to Ollama".into())),
        }
    }
}

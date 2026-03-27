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
    async fn async_prompt(&self, object_id: &str, prompt: &str) -> Result<(), LLMError> {
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

        let object_id = object_id.to_owned();
        tokio::spawn(async move {
            let response_content = match client.post(&url).json(&body).send().await {
                Ok(response) => match response.json::<serde_json::Value>().await {
                    Ok(json) => json["message"]["content"].as_str().map(|content| content.to_owned()),
                    Err(_) => None,
                },
                Err(_) => None,
            };

            if let (Some(cb), Some(content)) = (callback, response_content) {
                cb(object_id, content);
            }
        });
        Ok(())
    }

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

        let response = self.client.post(&url).json(&body).send().await.map_err(|e| LLMError::ConnectionError(e.to_string()))?;
        let json: serde_json::Value = response.json().await.map_err(|e| LLMError::GenerationError(e.to_string()))?;
        json["message"]["content"].as_str().map(|s| s.to_owned()).ok_or_else(|| LLMError::GenerationError("Invalid response format".to_string()))
    }

    fn set_callback(&mut self, cb: Callback) {
        self.callback.replace(cb);
    }
}

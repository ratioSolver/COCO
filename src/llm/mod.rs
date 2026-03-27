use async_trait::async_trait;
use std::sync::Arc;

#[cfg(feature = "ollama")]
mod ollama;

#[derive(Debug)]
pub enum LLMError {
    ConnectionError(String),
    GenerationError(String),
}

pub type Callback = Arc<dyn Fn(String, String) + Send + Sync + 'static>;

#[async_trait]
pub trait LLM: Send + Sync {
    async fn async_prompt(&self, object_id: &str, prompt: &str) -> Result<(), LLMError>;
    async fn prompt(&self, prompt: &str) -> Result<String, LLMError>;

    fn set_callback(&mut self, cb: Callback);
}

pub fn setup_llm() -> Option<Arc<dyn LLM>> {
    #[cfg(feature = "ollama")]
    return Some(setup_ollama());

    #[cfg(not(feature = "ollama"))]
    None
}

#[cfg(feature = "ollama")]
fn setup_ollama() -> Arc<dyn LLM> {
    use crate::llm::ollama::Ollama;

    let host = std::env::var("LLM_HOST").unwrap_or_else(|_| "localhost".to_owned());
    let port = std::env::var("LLM_PORT").ok().and_then(|p| p.parse().ok()).unwrap_or(11434);
    let model = std::env::var("LLM_MODEL").unwrap_or_else(|_| "llama3".to_owned());
    Arc::new(Ollama::new(host, port, model))
}

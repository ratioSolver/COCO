use async_trait::async_trait;

#[cfg(feature = "ollama")]
mod ollama;

#[derive(Debug)]
pub enum LLMError {
    ConnectionError(String),
    GenerationError(String),
}

#[async_trait]
pub trait LLM: Send + Sync {
    async fn prompt(&self, prompt: &str) -> Result<String, LLMError>;
}

pub fn setup_llm() -> Option<Box<dyn LLM>> {
    #[cfg(feature = "ollama")]
    return Some(setup_ollama());

    #[cfg(not(feature = "ollama"))]
    None
}

#[cfg(feature = "ollama")]
fn setup_ollama() -> Box<dyn LLM> {
    use crate::llm::ollama::Ollama;

    let host = std::env::var("LLM_HOST").unwrap_or_else(|_| "localhost".to_string());
    let port = std::env::var("LLM_PORT").ok().and_then(|p| p.parse().ok()).unwrap_or(11434);
    let model = std::env::var("LLM_MODEL").unwrap_or_else(|_| "llama3".to_string());
    Box::new(Ollama::new(host, port, model))
}

use async_trait::async_trait;

#[cfg(feature = "ollama")]
pub mod ollama;

#[derive(Debug)]
pub enum LLMError {
    ConnectionError(String),
    GenerationError(String),
}

#[async_trait]
pub trait LLM: Send + Sync {
    async fn propmt(&self, prompt: &str) -> Result<String, LLMError>;
}

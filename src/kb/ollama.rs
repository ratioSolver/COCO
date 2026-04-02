use crate::kb::{KnowledgeBaseError, clips::CLIPSKnowledgeBase};
use clips::{ClipsValue, Type};
use reqwest::Client;

pub fn add_ollama(kb: &CLIPSKnowledgeBase, host: String, port: u16, model: String) -> Result<(), KnowledgeBaseError> {
    let url = format!("http://{}:{}/api/chat", host, port);
    let client = Client::new();
    let udf_client = client.clone();

    kb.build("(deftemplate llm-result (slot item_id (type SYMBOL)) (slot result (type STRING)))")?;
    kb.add_udf(
        "prompt",
        None,
        2,
        2,
        vec![Type(Type::SYMBOL), Type(Type::STRING)],
        Box::new(move |_env, ctx| {
            let object_id = ctx.get_next_argument(Type(Type::SYMBOL)).expect("Failed to get object ID argument for prompt UDF");
            let object_id = if let ClipsValue::Symbol(s) = object_id { s } else { panic!("Expected symbol for object ID argument in prompt UDF") };
            let prompt = ctx.get_next_argument(Type(Type::STRING)).expect("Failed to get prompt argument for prompt UDF");
            let prompt = if let ClipsValue::String(s) = prompt { s } else { panic!("Expected string for prompt argument in prompt UDF") };

            let client = udf_client.clone();
            let url = url.clone();
            let model = model.clone();

            tokio::spawn(async move {
                let body = serde_json::json!({
                    "model": model,
                    "messages": [
                        {
                            "role": "user",
                            "content": prompt
                        }
                    ],
                    "stream": false
                });

                let _response_content = match client.post(&url).json(&body).send().await {
                    Ok(response) => match response.json::<serde_json::Value>().await {
                        Ok(json) => json["message"]["content"].as_str().map(|content| content.to_owned()),
                        Err(_) => None,
                    },
                    Err(_) => None,
                };
            });

            ClipsValue::Void()
        }),
    )?;

    Ok(())
}

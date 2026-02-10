use crate::kb::KnowledgeBase;
use clips::{ClipsValue, Environment, Type};

pub struct CLIPSKnowledgeBase {
    env: Environment,
}

impl CLIPSKnowledgeBase {
    pub fn new() -> Self {
        let mut env = Environment::new().expect("Failed to create CLIPS environment");
        env.add_udf("add-data", None, 3, 4, vec![Type(Type::SYMBOL), Type(Type::MULTIFIELD), Type(Type::MULTIFIELD), Type(Type::INTEGER)], |_env, _ctx| ClipsValue::Void()).expect("Failed to add UDF to CLIPS environment");
        env.add_udf("add-class", None, 2, 2, vec![Type(Type::SYMBOL), Type(Type::SYMBOL)], |_env, _ctx| ClipsValue::Void()).expect("Failed to add UDF to CLIPS environment");
        Self { env }
    }
}

impl KnowledgeBase for CLIPSKnowledgeBase {}

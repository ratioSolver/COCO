use crate::{
    kb::{KnowledgeBase, KnowledgeBaseError},
    model::Class,
};
use clips::{ClipsValue, Environment, Type};
use futures::lock::Mutex;
use std::collections::HashMap;

pub struct CLIPSKnowledgeBase {
    env: Mutex<Environment>,
    classes: HashMap<String, Class>,
}

unsafe impl Send for CLIPSKnowledgeBase {}
unsafe impl Sync for CLIPSKnowledgeBase {}

impl CLIPSKnowledgeBase {
    pub fn new() -> Self {
        let mut env = Environment::new().expect("Failed to create CLIPS environment");
        env.add_udf("add-data", None, 3, 4, vec![Type(Type::SYMBOL), Type(Type::MULTIFIELD), Type(Type::MULTIFIELD), Type(Type::INTEGER)], |_env, _ctx| ClipsValue::Void()).expect("Failed to add UDF to CLIPS environment");
        env.add_udf("add-class", None, 2, 2, vec![Type(Type::SYMBOL), Type(Type::SYMBOL)], |_env, _ctx| ClipsValue::Void()).expect("Failed to add UDF to CLIPS environment");
        Self { env: Mutex::new(env), classes: HashMap::new() }
    }
}

impl KnowledgeBase for CLIPSKnowledgeBase {
    fn get_classes(&self) -> Vec<&Class> {
        self.classes.values().collect()
    }

    fn get_class(&self, name: &str) -> Option<&Class> {
        self.classes.get(name)
    }

    fn create_class(&mut self, class: &Class) -> Result<(), KnowledgeBaseError> {
        if self.classes.contains_key(&class.name) {
            return Err(KnowledgeBaseError::ClassAlreadyExists(class.name.clone()));
        }
        self.classes.insert(class.name.clone(), class.clone());
        Ok(())
    }
}

use crate::kb::KnowledgeBase;

pub struct CLIPSKnowledgeBase {}

impl CLIPSKnowledgeBase {
    pub fn new() -> Self {
        Self {}
    }
}

impl KnowledgeBase for CLIPSKnowledgeBase {}

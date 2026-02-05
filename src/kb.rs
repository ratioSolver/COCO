pub trait KnowledgeBase: Send + Sync {
    fn add_class(&mut self, class_name: &str);
}

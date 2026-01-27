use crate::db::DBRule;

pub struct Rule {
    name: String,
    content: String,
}

impl Rule {
    pub fn new(name: &str, content: &str) -> Self {
        Self {
            name: name.to_string(),
            content: content.to_string(),
        }
    }

    pub fn from_db_rule(db_rule: DBRule) -> Self {
        Self {
            name: db_rule.name,
            content: db_rule.content,
        }
    }

    pub fn name(&self) -> &str {
        &self.name
    }

    pub fn content(&self) -> &str {
        &self.content
    }
}

use crate::db::DBRule;

pub struct Rule {
    name: String,
    content: String,
}

impl Rule {
    pub(super) fn new(db_rule: DBRule) -> Self {
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

pub struct Rule {
    name: String,
}

impl Rule {
    pub fn new(name: &str) -> Self {
        Self {
            name: name.to_string(),
        }
    }
}

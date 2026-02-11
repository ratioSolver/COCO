#[cfg(feature = "mongodb")]
pub mod mongodb;

#[derive(Debug)]
pub enum DatabaseError {
    ConnectionError(String),
}

pub trait Database {
    fn name(&self) -> &str;
}

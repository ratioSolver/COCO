pub trait Database {}

#[cfg(feature = "mongodb")]
pub mod mongodb;

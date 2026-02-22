use chrono::{Duration, Utc};
use jsonwebtoken::{EncodingKey, Header, encode, errors::Error};
use serde::{Deserialize, Serialize};

#[derive(Debug, Serialize, Deserialize)]
struct Claims {
    pub sub: String,
    pub exp: usize,
    pub role: String,
}

pub fn create_jwt(user_id: &str, secret: &str) -> Result<String, Error> {
    let now = Utc::now();
    let expire = now + Duration::hours(24);

    let claims = Claims { sub: user_id.to_owned(), exp: expire.timestamp() as usize, role: "user".to_string() };

    encode(&Header::default(), &claims, &EncodingKey::from_secret(secret.as_ref()))
}

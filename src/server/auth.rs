use argon2::{
    Argon2, PasswordHash, PasswordHasher, PasswordVerifier,
    password_hash::{SaltString, rand_core::OsRng},
};
use axum::{
    extract::{Request, State},
    http::{StatusCode, header},
    middleware::Next,
    response::Response,
};
use chrono::{Duration, Utc};
use jsonwebtoken::{DecodingKey, EncodingKey, Header, Validation, errors::Error};
use serde::{Deserialize, Serialize};

use crate::server::CoCoState;

fn hash_password(password: &str) -> String {
    let salt = SaltString::generate(&mut OsRng);
    Argon2::default().hash_password(password.as_bytes(), &salt).unwrap().to_string()
}

fn verify_password(password: &str, hash: &str) -> bool {
    let parsed_hash = PasswordHash::new(hash).unwrap();
    Argon2::default().verify_password(password.as_bytes(), &parsed_hash).is_ok()
}

#[derive(Debug, Serialize, Deserialize)]
struct Claims {
    sub: String,
    exp: usize,
    role: String,
}

fn create_jwt(user_id: &str, role: &str, secret: &str) -> Result<String, Error> {
    let now = Utc::now();
    let expire = now + Duration::hours(24);

    let claims = Claims { sub: user_id.to_owned(), exp: expire.timestamp() as usize, role: role.to_owned() };

    jsonwebtoken::encode(&Header::default(), &claims, &EncodingKey::from_secret(secret.as_ref()))
}

fn verify_jwt(token: &str, secret: String) -> Result<Claims, Error> {
    let decoding_key = DecodingKey::from_secret(secret.as_ref());
    let validation = Validation::default();
    let token_data = jsonwebtoken::decode::<Claims>(token, &decoding_key, &validation)?;
    Ok(token_data.claims)
}

#[derive(Deserialize)]
struct RegisterRequest {
    username: String,
    password: String,
    role: String,
}

#[derive(Deserialize)]
struct LoginRequest {
    username: String,
    password: String,
}

#[derive(Debug, Clone)]
struct CurrentUser {
    id: String,
    role: String,
}

async fn register<S: CoCoState>(State(state): State<S>, req: RegisterRequest, secret: &str) -> Result<String, String> {
    let _hashed = hash_password(&req.password);
    create_jwt(&req.username, &req.role, secret).map_err(|e| e.to_string())
}

async fn login<S: CoCoState>(State(state): State<S>, req: LoginRequest, secret: &str) -> Result<String, String> {
    let _hashed = hash_password(&req.password);
    create_jwt(&req.username, "user", secret).map_err(|e| e.to_string())
}

async fn auth_middleware<S: CoCoState>(State(state): State<S>, mut req: Request, next: Next) -> Result<Response, StatusCode> {
    let header = req.headers().get(header::AUTHORIZATION).and_then(|h| h.to_str().ok());
    if let Some(token) = header.and_then(|h| h.strip_prefix("Bearer ")) {
        let secret = std::env::var("JWT_SECRET").unwrap_or_else(|_| "default_secret".to_string());
        if let Ok(claims) = verify_jwt(token, secret) {
            req.extensions_mut().insert(CurrentUser { id: claims.sub, role: claims.role });
            return Ok(next.run(req).await);
        }
    }
    Err(StatusCode::UNAUTHORIZED)
}

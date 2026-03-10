use crate::server::CoCoState;
use argon2::{
    Argon2, PasswordHash, PasswordHasher, PasswordVerifier,
    password_hash::{SaltString, rand_core::OsRng},
};
use async_trait::async_trait;
use axum::{
    Json,
    extract::{Request, State},
    http::{StatusCode, header},
    middleware::Next,
    response::{IntoResponse, Response},
};
use chrono::{Duration, Utc};
use jsonwebtoken::{DecodingKey, EncodingKey, Header, Validation, errors::Error};
use serde::{Deserialize, Serialize};

#[async_trait]
pub trait SecureCoCoState: CoCoState {
    fn secret(&self) -> &str;

    async fn login(&self, username: &str, hashed_password: &str) -> Option<String>;
    async fn register(&self, username: &str, hashed_password: &str, role: &str) -> bool;
}

pub fn hash_password(password: &str) -> String {
    let salt = SaltString::generate(&mut OsRng);
    Argon2::default().hash_password(password.as_bytes(), &salt).unwrap().to_string()
}

pub fn verify_password(password: &str, hash: &str) -> bool {
    let parsed_hash = PasswordHash::new(hash).unwrap();
    Argon2::default().verify_password(password.as_bytes(), &parsed_hash).is_ok()
}

#[derive(Debug, Serialize, Deserialize)]
struct Claims {
    sub: String,
    exp: usize,
    role: String,
}

pub fn create_jwt(user_id: &str, role: &str, secret: &str) -> Result<String, Error> {
    let now = Utc::now();
    let expire = now + Duration::hours(24);

    let claims = Claims { sub: user_id.to_owned(), exp: expire.timestamp() as usize, role: role.to_owned() };

    jsonwebtoken::encode(&Header::default(), &claims, &EncodingKey::from_secret(secret.as_ref()))
}

pub fn verify_jwt(token: &str, secret: String) -> Result<Claims, Error> {
    let decoding_key = DecodingKey::from_secret(secret.as_ref());
    let validation = Validation::default();
    let token_data = jsonwebtoken::decode::<Claims>(token, &decoding_key, &validation)?;
    Ok(token_data.claims)
}

#[derive(Deserialize)]
pub(super) struct RegisterRequest {
    username: String,
    password: String,
    role: String,
}

#[derive(Deserialize)]
pub(super) struct LoginRequest {
    username: String,
    password: String,
}

#[derive(Debug, Clone)]
struct CurrentUser {
    id: String,
    role: String,
}

pub(super) async fn register<S: SecureCoCoState>(State(state): State<S>, Json(req): Json<RegisterRequest>) -> impl IntoResponse {
    if !state.register(&req.username, &hash_password(&req.password), &req.role).await {
        return Err(StatusCode::CONFLICT);
    }
    create_jwt(&req.username, &req.role, state.secret()).map_err(|e| e.to_string()).map_err(|_| StatusCode::INTERNAL_SERVER_ERROR)
}

pub(super) async fn login<S: SecureCoCoState>(State(state): State<S>, Json(req): Json<LoginRequest>) -> impl IntoResponse {
    let role = state.login(&req.username, &hash_password(&req.password)).await.ok_or(StatusCode::UNAUTHORIZED)?;
    create_jwt(&req.username, role.as_str(), state.secret()).map_err(|e| e.to_string()).map_err(|_| StatusCode::INTERNAL_SERVER_ERROR)
}

pub(super) async fn auth_middleware<S: SecureCoCoState>(State(state): State<S>, mut req: Request, next: Next) -> Result<Response, StatusCode> {
    let header = req.headers().get(header::AUTHORIZATION).and_then(|h| h.to_str().ok());
    if let Some(token) = header.and_then(|h| h.strip_prefix("Bearer ")) {
        if let Ok(claims) = verify_jwt(token, state.secret().to_string()) {
            req.extensions_mut().insert(CurrentUser { id: claims.sub, role: claims.role });
            return Ok(next.run(req).await);
        }
    }
    Err(StatusCode::UNAUTHORIZED)
}

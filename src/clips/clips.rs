use crate::db::Class;
use crate::kb::KnowledgeBase as KnowledgeBaseTrait;
use std::error::Error;
use std::ffi::c_long;
use std::marker::{PhantomData, PhantomPinned};

#[repr(C)]
pub struct Environment {
    _data: [u8; 0],
    _marker: PhantomData<(*mut u8, PhantomPinned)>,
}

#[repr(C)]
#[derive(Debug)]
#[allow(dead_code)]
enum BuildError {
    NoError,
    CouldNotBuildError,
    ConstructNotFoundError,
    ParsingError,
}

#[link(name = "clips")]
#[allow(dead_code)]
unsafe extern "C" {
    unsafe fn CreateEnvironment() -> *mut Environment;
    unsafe fn DestroyEnvironment(env: *mut Environment);
    unsafe fn Build(env: *mut Environment, construct: *const i8) -> BuildError;
    unsafe fn Run(env: *mut Environment, runLimit: c_long) -> c_long;
}

pub struct KnowledgeBase {
    env: *mut Environment,
}

impl KnowledgeBase {
    pub fn new() -> Self {
        unsafe {
            let env = CreateEnvironment();
            KnowledgeBase { env }
        }
    }
}

impl Drop for KnowledgeBase {
    fn drop(&mut self) {
        unsafe {
            DestroyEnvironment(self.env);
        }
    }
}

impl KnowledgeBaseTrait for KnowledgeBase {
    fn create_class(&self, class: &Class) -> Result<(), Box<dyn Error>> {
        let deftemplate = format!("(deftemplate {})", class.name);
        let c_str = std::ffi::CString::new(deftemplate)?;
        unsafe {
            let result = Build(self.env, c_str.as_ptr());
            match result {
                BuildError::NoError => Ok(()),
                _ => Err(format!("Build error: {:?}", result).into()),
            }
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_create_and_destroy_env() {
        let kb = KnowledgeBase::new();
        assert!(!kb.env.is_null());
    }

    #[test]
    fn test_create_class() {
        let kb = KnowledgeBase::new();
        let class = Class {
            name: "TestClass".to_string(),
            parents: None,
            static_properties: None,
            dynamic_properties: None,
        };
        let result = kb.create_class(&class);
        assert!(result.is_ok(), "Failed to create class: {:?}", result.err());
    }
}

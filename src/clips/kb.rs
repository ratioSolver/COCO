use chrono::{DateTime, Utc};

use crate::{Class, KnowledgeBase as KnowledgeBaseTrait, Object, Property, Value};
use std::collections::HashMap;
use std::error::Error;
use std::ffi::{c_char, c_double, c_long, c_longlong, c_ushort};
use std::marker::{PhantomData, PhantomPinned};
use std::os::raw::c_void;

#[repr(C)]
pub struct Environment {
    _data: [u8; 0],
    _marker: PhantomData<(*mut u8, PhantomPinned)>,
}

#[repr(C)]
struct FactBuilder {
    _data: [u8; 0],
    _marker: PhantomData<(*mut u8, PhantomPinned)>,
}

#[repr(C)]
struct FactModifier {
    _data: [u8; 0],
    _marker: PhantomData<(*mut u8, PhantomPinned)>,
}

#[repr(C)]
struct Fact {
    _data: [u8; 0],
    _marker: PhantomData<(*mut u8, PhantomPinned)>,
}

#[repr(C)]
pub struct TypeHeader {
    pub type_code: c_ushort,
}

#[repr(C)]
pub struct CLIPSLexeme {
    pub header: TypeHeader,
    pub _marker: PhantomData<(*mut u8, PhantomPinned)>,
}

#[repr(C)]
pub struct CLIPSFloat {
    pub header: TypeHeader,
    pub contents: c_double,
}

#[repr(C)]
pub struct CLIPSInteger {
    pub header: TypeHeader,
    pub contents: c_longlong,
}

#[repr(C)]
pub struct CLIPSVoid {
    pub header: TypeHeader,
}

#[repr(C)]
struct Multifield {
    _data: [u8; 0],
    _marker: PhantomData<(*mut u8, PhantomPinned)>,
}

#[repr(C)]
struct Instance {
    _data: [u8; 0],
    _marker: PhantomData<(*mut u8, PhantomPinned)>,
}

#[repr(C)]
struct CLIPSExternalAddress {
    _data: [u8; 0],
    _marker: PhantomData<(*mut u8, PhantomPinned)>,
}

#[repr(C)]
pub struct UDFContext {
    context: *mut c_void,
}

#[repr(C)]
pub union UDFValueUnion {
    value: *mut c_void,
    header: *mut TypeHeader,
    lexeme_value: *mut CLIPSLexeme,
    float_value: *mut CLIPSFloat,
    integer_value: *mut CLIPSInteger,
    void_value: *mut CLIPSVoid,
    multifield_value: *mut Multifield,
    fact_value: *mut Fact,
    instance_value: *mut Instance,
    external_address_value: *mut CLIPSExternalAddress,
}

#[repr(C)]
pub struct UDFValue {
    value: UDFValueUnion,
    begin: usize,
    range: usize,
}

#[allow(dead_code)]
type UserDefinedFunction = unsafe extern "C" fn(env: *mut Environment, udfc: *mut UDFContext, out: *mut UDFValue);

#[repr(C)]
#[derive(Debug)]
#[allow(dead_code)]
enum BuildError {
    None,
    CouldNotBuild,
    ConstructNotFound,
    Parsing,
}

#[repr(C)]
#[derive(Debug)]
#[allow(dead_code)]
enum FactBuilderError {
    None,
    NullPointer,
    DeftemplateNotFound,
    ImpliedDeftemplate,
    CouldNotAssert,
    RuleNetwork,
}

#[repr(C)]
#[derive(Debug)]
#[allow(dead_code)]
enum FactModifierError {
    None,
    NullPointer,
    Retracted,
    ImpliedDeftemplate,
    CouldNotModify,
    RuleNetwork,
}

#[repr(C)]
#[derive(Debug)]
#[allow(dead_code)]
enum PutSlotError {
    None,
    NullPointer,
    InvalidTarget,
    SlotNotFound,
    Type,
    Range,
    AllowedValues,
    Cardinality,
    AllowedClasses,
}

#[repr(C)]
#[derive(Debug)]
#[allow(dead_code)]
enum AddUDFError {
    None,
    MinExceedsMax,
    FunctionNameInUse,
    InvalidArgumentType,
    InvalidReturnType,
}

#[link(name = "clips")]
#[allow(dead_code)]
unsafe extern "C" {
    unsafe fn CreateEnvironment() -> *mut Environment;
    unsafe fn DestroyEnvironment(env: *mut Environment);
    unsafe fn Build(env: *mut Environment, construct: *const c_char) -> BuildError;
    unsafe fn CreateFactBuilder(env: *mut Environment, template_name: *const c_char) -> *mut FactBuilder;
    unsafe fn FBAssert(fb: *mut FactBuilder) -> *mut Fact;
    unsafe fn FBDispose(fb: *mut FactBuilder);
    unsafe fn FBError(fb: *mut FactBuilder) -> FactBuilderError;
    unsafe fn CreateFactModifier(env: *mut Environment, fact: *mut Fact) -> *mut FactModifier;
    unsafe fn FMModify(fm: *mut FactModifier) -> *mut Fact;
    unsafe fn FMDispose(fm: *mut FactModifier);
    unsafe fn FMError(fm: *mut FactModifier) -> FactModifierError;
    unsafe fn FBPutSlotInteger(fb: *mut FactBuilder, slot_name: *const c_char, value: c_longlong) -> PutSlotError;
    unsafe fn FBPutSlotFloat(fb: *mut FactBuilder, slot_name: *const c_char, value: c_double) -> PutSlotError;
    unsafe fn FBPutSlotSymbol(fb: *mut FactBuilder, slot_name: *const c_char, value: *const c_char) -> PutSlotError;
    unsafe fn FBPutSlotString(fb: *mut FactBuilder, slot_name: *const c_char, value: *const c_char) -> PutSlotError;
    unsafe fn FMPutSlotInteger(fm: *mut FactModifier, slot_name: *const c_char, value: c_longlong) -> PutSlotError;
    unsafe fn FMPutSlotFloat(fm: *mut FactModifier, slot_name: *const c_char, value: c_double) -> PutSlotError;
    unsafe fn FMPutSlotSymbol(fm: *mut FactModifier, slot_name: *const c_char, value: *const c_char) -> PutSlotError;
    unsafe fn FMPutSlotString(fm: *mut FactModifier, slot_name: *const c_char, value: *const c_char) -> PutSlotError;
    unsafe fn AddUDF(env: *mut Environment, name: *const c_char, return_types: *const c_char, min_args: c_ushort, max_args: c_ushort, arg_types: *const c_char, function_ptr: UserDefinedFunction, r_name: *const c_char, context: *mut c_void) -> AddUDFError;
    unsafe fn Run(env: *mut Environment, run_limit: c_long) -> c_long;
}

pub struct KnowledgeBase {
    env: *mut Environment,
    instances: HashMap<String, HashMap<String, *mut Fact>>,
}
unsafe impl Send for KnowledgeBase {}

impl Default for KnowledgeBase {
    fn default() -> Self {
        Self::new()
    }
}

impl KnowledgeBase {
    pub fn new() -> Self {
        unsafe {
            let env = CreateEnvironment();
            KnowledgeBase { env, instances: HashMap::new() }
        }
    }

    pub fn add_udf(&self, name: &str, return_types: &str, min_args: u16, max_args: u16, arg_types: &str, function_ptr: UserDefinedFunction, r_name: &str) -> Result<(), Box<dyn Error>> {
        unsafe {
            let result = AddUDF(self.env, std::ffi::CString::new(name)?.as_ptr(), std::ffi::CString::new(return_types)?.as_ptr(), min_args, max_args, std::ffi::CString::new(arg_types)?.as_ptr(), function_ptr, std::ffi::CString::new(r_name)?.as_ptr(), self as *const _ as *mut c_void);
            match result {
                AddUDFError::None => Ok(()),
                _ => Err(format!("AddUDF error: {:?}", result).into()),
            }
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
        let mut slots = String::new();
        if let Some(static_props) = &class.static_properties {
            for (name, prop) in static_props {
                slots.push_str(&format!(" {}", prop_slot(name, prop)));
            }
        }
        if let Some(dynamic_props) = &class.dynamic_properties {
            for (name, prop) in dynamic_props {
                slots.push_str(&format!(" {}", prop_slot(name, prop)));
            }
        }
        let deftemplate = format!("(deftemplate {} {} (slot id (type SYMBOL)))", class.name, slots);
        unsafe {
            let result = Build(self.env, std::ffi::CString::new(deftemplate)?.as_ptr());
            match result {
                BuildError::None => Ok(()),
                _ => Err(format!("Build error: {:?}", result).into()),
            }
        }
    }

    fn create_object(&mut self, class: &Class, object: &Object) -> Result<(), Box<dyn Error>> {
        unsafe {
            let fb = CreateFactBuilder(self.env, std::ffi::CString::new(class.name.clone())?.as_ptr());
            if fb.is_null() {
                return Err("Failed to create FactBuilder".into());
            }

            match FBPutSlotSymbol(fb, std::ffi::CString::new("id")?.as_ptr(), std::ffi::CString::new(object.id.clone())?.as_ptr()) {
                PutSlotError::None => {}
                err => {
                    FBDispose(fb);
                    return Err(format!("PutSlot error: {:?}", err).into());
                }
            }

            for (prop_name, value) in object.properties.as_ref().unwrap_or(&HashMap::new()) {
                match (class.static_properties.as_ref().ok_or("Class has no static properties")?.get(prop_name).unwrap(), value) {
                    (Property::Bool { nullable, .. }, Value::Null) => {
                        if let Some(true) = nullable {
                            match FBPutSlotSymbol(fb, std::ffi::CString::new(prop_name.clone())?.as_ptr(), std::ffi::CString::new("nil")?.as_ptr()) {
                                PutSlotError::None => {}
                                err => {
                                    FBDispose(fb);
                                    return Err(format!("PutSlot error: {:?}", err).into());
                                }
                            }
                        } else {
                            FBDispose(fb);
                            return Err("Cannot assign null to non-nullable property".into());
                        }
                    }
                    (Property::Bool { .. }, Value::Bool(b)) => {
                        let symbol = if *b { "TRUE" } else { "FALSE" };
                        match FBPutSlotSymbol(fb, std::ffi::CString::new(prop_name.clone())?.as_ptr(), std::ffi::CString::new(symbol)?.as_ptr()) {
                            PutSlotError::None => {}
                            err => {
                                FBDispose(fb);
                                return Err(format!("PutSlot error: {:?}", err).into());
                            }
                        }
                    }
                    (Property::Int { nullable, .. }, Value::Null) => {
                        if let Some(true) = nullable {
                            match FBPutSlotSymbol(fb, std::ffi::CString::new(prop_name.clone())?.as_ptr(), std::ffi::CString::new("nil")?.as_ptr()) {
                                PutSlotError::None => {}
                                err => {
                                    FBDispose(fb);
                                    return Err(format!("PutSlot error: {:?}", err).into());
                                }
                            }
                        } else {
                            FBDispose(fb);
                            return Err("Cannot assign null to non-nullable property".into());
                        }
                    }
                    (Property::Int { .. }, Value::Int(i)) => match FBPutSlotInteger(fb, std::ffi::CString::new(prop_name.clone())?.as_ptr(), *i) {
                        PutSlotError::None => {}
                        err => {
                            FBDispose(fb);
                            return Err(format!("PutSlot error: {:?}", err).into());
                        }
                    },
                    (Property::Float { nullable, .. }, Value::Null) => {
                        if let Some(true) = nullable {
                            match FBPutSlotSymbol(fb, std::ffi::CString::new(prop_name.clone())?.as_ptr(), std::ffi::CString::new("nil")?.as_ptr()) {
                                PutSlotError::None => {}
                                err => {
                                    FBDispose(fb);
                                    return Err(format!("PutSlot error: {:?}", err).into());
                                }
                            }
                        } else {
                            FBDispose(fb);
                            return Err("Cannot assign null to non-nullable property".into());
                        }
                    }
                    (Property::Float { .. }, Value::Float(f)) => match FBPutSlotFloat(fb, std::ffi::CString::new(prop_name.clone())?.as_ptr(), *f) {
                        PutSlotError::None => {}
                        err => {
                            FBDispose(fb);
                            return Err(format!("PutSlot error: {:?}", err).into());
                        }
                    },
                    _ => {
                        FBDispose(fb);
                        return Err("Property type and value type mismatch".into());
                    }
                }
            }

            for (prop_name, value) in object.values.as_ref().unwrap_or(&HashMap::new()) {
                match (class.dynamic_properties.as_ref().ok_or("Class has no dynamic properties")?.get(prop_name).unwrap(), value) {
                    (Property::Bool { nullable, .. }, (Value::Null, _)) => {
                        if let Some(true) = nullable {
                            match FBPutSlotSymbol(fb, std::ffi::CString::new(prop_name.clone())?.as_ptr(), std::ffi::CString::new("nil")?.as_ptr()) {
                                PutSlotError::None => {}
                                err => {
                                    FBDispose(fb);
                                    return Err(format!("PutSlot error: {:?}", err).into());
                                }
                            }
                        } else {
                            FBDispose(fb);
                            return Err("Cannot assign null to non-nullable property".into());
                        }
                    }
                    (Property::Bool { .. }, (Value::Bool(b), _)) => {
                        let symbol = if *b { "TRUE" } else { "FALSE" };
                        match FBPutSlotSymbol(fb, std::ffi::CString::new(prop_name.clone())?.as_ptr(), std::ffi::CString::new(symbol)?.as_ptr()) {
                            PutSlotError::None => {}
                            err => {
                                FBDispose(fb);
                                return Err(format!("PutSlot error: {:?}", err).into());
                            }
                        }
                    }
                    (Property::Int { nullable, .. }, (Value::Null, _)) => {
                        if let Some(true) = nullable {
                            match FBPutSlotSymbol(fb, std::ffi::CString::new(prop_name.clone())?.as_ptr(), std::ffi::CString::new("nil")?.as_ptr()) {
                                PutSlotError::None => {}
                                err => {
                                    FBDispose(fb);
                                    return Err(format!("PutSlot error: {:?}", err).into());
                                }
                            }
                        } else {
                            FBDispose(fb);
                            return Err("Cannot assign null to non-nullable property".into());
                        }
                    }
                    (Property::Int { .. }, (Value::Int(i), _)) => match FBPutSlotInteger(fb, std::ffi::CString::new(prop_name.clone())?.as_ptr(), *i) {
                        PutSlotError::None => {}
                        err => {
                            FBDispose(fb);
                            return Err(format!("PutSlot error: {:?}", err).into());
                        }
                    },
                    (Property::Float { nullable, .. }, (Value::Null, _)) => {
                        if let Some(true) = nullable {
                            match FBPutSlotSymbol(fb, std::ffi::CString::new(prop_name.clone())?.as_ptr(), std::ffi::CString::new("nil")?.as_ptr()) {
                                PutSlotError::None => {}
                                err => {
                                    FBDispose(fb);
                                    return Err(format!("PutSlot error: {:?}", err).into());
                                }
                            }
                        } else {
                            FBDispose(fb);
                            return Err("Cannot assign null to non-nullable property".into());
                        }
                    }
                    (Property::Float { .. }, (Value::Float(f), _)) => match FBPutSlotFloat(fb, std::ffi::CString::new(prop_name.clone())?.as_ptr(), *f) {
                        PutSlotError::None => {}
                        err => {
                            FBDispose(fb);
                            return Err(format!("PutSlot error: {:?}", err).into());
                        }
                    },
                    _ => {
                        FBDispose(fb);
                        return Err("Property type and value type mismatch".into());
                    }
                }
            }

            let fact = FBAssert(fb);
            if fact.is_null() {
                let error = FBError(fb);
                FBDispose(fb);
                return Err(format!("Assertion failed: {:?}", error).into());
            }

            self.instances.entry(class.name.clone()).or_default().insert(object.id.clone(), fact);

            FBDispose(fb);
            Ok(())
        }
    }

    fn add_data(&mut self, class: &Class, object: &Object, values: Vec<(&str, &Value)>, date_time: DateTime<Utc>) -> Result<(), Box<dyn Error>> {
        unsafe {
            let fact = self.instances.get(&class.name).and_then(|objs| objs.get(&object.id)).ok_or("Object not found in knowledge base")?;
            let fm = CreateFactModifier(self.env, *fact);
            if fm.is_null() {
                return Err("Failed to create FactModifier".into());
            }
            for (prop_name, value) in values {
                match (class.dynamic_properties.as_ref().ok_or("Class has no dynamic properties")?.get(prop_name).unwrap(), value) {
                    _ => {}
                }
            }
            Ok(())
        }
    }
}

fn prop_slot(name: &str, property: &Property) -> String {
    match property {
        Property::Bool { nullable, default } => {
            let mut def = format!("(slot {} (type SYMBOL) (allowed-symbols TRUE FALSE", name);
            if let Some(true) = nullable {
                def.push_str(" nil");
            }
            def.push(')');
            if let Some(def_val) = default {
                def.push_str(&format!(" (default {})", if *def_val { "TRUE" } else { "FALSE" }));
            }
            def.push(')');
            def
        }
        Property::Int { nullable, default, min, max } => {
            let mut def = format!("(slot {} (type INTEGER", name);
            if let Some(true) = nullable {
                def.push_str(" SYMBOL) (allowed-symbols nil");
            }
            def.push(')');
            if let Some(def_val) = default {
                def.push_str(&format!(" (default {})", def_val));
            }
            if (min.is_some() || max.is_some()) && !nullable.unwrap_or(false) {
                let min_str = min.map(|v| v.to_string()).unwrap_or("?VARIABLE".to_string());
                let max_str = max.map(|v| v.to_string()).unwrap_or("?VARIABLE".to_string());
                def.push_str(&format!(" (range {} {})", min_str, max_str));
            }
            def.push(')');
            def
        }
        Property::Float { nullable, default, min, max } => {
            let mut def = format!("(slot {} (type FLOAT", name);
            if let Some(true) = nullable {
                def.push_str(" SYMBOL) (allowed-symbols nil");
            }
            def.push(')');
            if let Some(def_val) = default {
                def.push_str(&format!(" (default {})", def_val));
            }
            if (min.is_some() || max.is_some()) && !nullable.unwrap_or(false) {
                let min_str = min
                    .map(|v| {
                        let s = v.to_string();
                        if s.contains('.') { s } else { format!("{}.0", s) }
                    })
                    .unwrap_or("?VARIABLE".to_string());
                let max_str = max
                    .map(|v| {
                        let s = v.to_string();
                        if s.contains('.') { s } else { format!("{}.0", s) }
                    })
                    .unwrap_or("?VARIABLE".to_string());
                def.push_str(&format!(" (range {} {})", min_str, max_str));
            }
            def.push(')');
            def
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

    #[test]
    fn test_create_object() {
        let mut kb = KnowledgeBase::new();
        let class = Class {
            name: "TestClass".to_string(),
            parents: None,
            static_properties: None,
            dynamic_properties: None,
        };
        let res_class = kb.create_class(&class);
        assert!(res_class.is_ok(), "Failed to create class: {:?}", res_class.err());

        let object = Object { id: "obj1".to_string(), classes: None, properties: None, values: None };
        let result = kb.create_object(&class, &object);
        assert!(result.is_ok(), "Failed to create object: {:?}", result.err());
    }

    #[test]
    fn test_create_class_with_properties() {
        let kb = KnowledgeBase::new();
        let mut static_props = HashMap::new();
        static_props.insert("is_valid".to_string(), Property::Bool { nullable: Some(false), default: Some(true) });
        static_props.insert("score".to_string(), Property::Int { nullable: Some(true), default: Some(10), min: Some(0), max: Some(100) });

        let mut dynamic_props = HashMap::new();
        dynamic_props.insert("temperature".to_string(), Property::Float { nullable: Some(false), default: Some(36.6), min: Some(30.0), max: Some(45.0) });

        let class = Class {
            name: "ComplexClass".to_string(),
            parents: None,
            static_properties: Some(static_props),
            dynamic_properties: Some(dynamic_props),
        };

        let result = kb.create_class(&class);
        assert!(result.is_ok(), "Failed to create class with properties: {:?}", result.err());
    }

    #[test]
    fn test_add_udf() {
        let kb = KnowledgeBase::new();

        unsafe extern "C" fn test_udf(_env: *mut Environment, _udfc: *mut UDFContext, _out: *mut UDFValue) {
            // Function logic placeholder
        }

        let result = kb.add_udf("test_function", "v", 0, 0, "", test_udf, "test_udf");

        assert!(result.is_ok(), "Failed to add UDF: {:?}", result.err());
    }
}

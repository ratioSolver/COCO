use chrono::{DateTime, Utc};

use crate::{Class, KnowledgeBase as KnowledgeBaseTrait, Object, Property, Value};
use std::collections::HashMap;
use std::error::Error;
use std::ffi::{CString, c_char, c_double, c_long, c_longlong, c_ushort};
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
    classes: HashMap<String, Class>,
    env: *mut Environment,
    data_callback: Box<dyn FnMut(&str, HashMap<String, Value>, DateTime<Utc>)>,
    instances: HashMap<String, HashMap<String, *mut Fact>>,              // class -> object -> fact
    facts: HashMap<String, HashMap<String, HashMap<String, *mut Fact>>>, // class -> object -> property -> fact
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
            KnowledgeBase {
                classes: HashMap::new(),
                env: CreateEnvironment(),
                data_callback: Box::new(|_, _, _| {}),
                instances: HashMap::new(),
                facts: HashMap::new(),
            }
        }
    }

    pub fn add_udf(&self, name: &str, return_types: &str, min_args: u16, max_args: u16, arg_types: &str, function_ptr: UserDefinedFunction, r_name: &str) -> Result<(), Box<dyn Error>> {
        unsafe {
            let result = AddUDF(self.env, CString::new(name)?.as_ptr(), CString::new(return_types)?.as_ptr(), min_args, max_args, CString::new(arg_types)?.as_ptr(), function_ptr, CString::new(r_name)?.as_ptr(), self as *const _ as *mut c_void);
            match result {
                AddUDFError::None => Ok(()),
                _ => Err(format!("AddUDF error: {:?}", result).into()),
            }
        }
    }

    fn set_prop(&mut self, object: &Object, class: &Class, property: &Property, property_name: &str, value: &Value, time: Option<&DateTime<Utc>>) -> Result<(), Box<dyn Error>> {
        unsafe {
            let fb = CreateFactBuilder(self.env, CString::new(format!("{}_{}", class.name, property_name))?.as_ptr());
            if fb.is_null() {
                return Err("Failed to create FactBuilder".into());
            }
            let handle_err = |msg: &str| {
                FBDispose(fb);
                Err::<(), Box<dyn Error>>(msg.to_string().into())
            };
            match FBPutSlotSymbol(fb, CString::new("id")?.as_ptr(), CString::new(object.id.clone())?.as_ptr()) {
                PutSlotError::None => {}
                err => handle_err(&format!("PutSlot error: {:?}", err))?,
            }
            match property {
                Property::Bool { nullable, .. } => match value {
                    Value::Null => {
                        if let Some(true) = nullable {
                            match FBPutSlotSymbol(fb, CString::new("value")?.as_ptr(), CString::new("nil")?.as_ptr()) {
                                PutSlotError::None => {}
                                err => handle_err(&format!("PutSlot error: {:?}", err))?,
                            }
                        } else {
                            return handle_err("Cannot assign null to non-nullable property");
                        }
                    }
                    Value::Bool(b) => {
                        let symbol = if *b { "TRUE" } else { "FALSE" };
                        match FBPutSlotSymbol(fb, CString::new("value")?.as_ptr(), CString::new(symbol)?.as_ptr()) {
                            PutSlotError::None => {}
                            err => handle_err(&format!("PutSlot error: {:?}", err))?,
                        }
                    }
                    _ => return handle_err("Property type and value type mismatch"),
                },
                Property::Int { nullable, min, max, .. } => match value {
                    Value::Null => {
                        if let Some(true) = nullable {
                            match FBPutSlotSymbol(fb, CString::new("value")?.as_ptr(), CString::new("nil")?.as_ptr()) {
                                PutSlotError::None => {}
                                err => handle_err(&format!("PutSlot error: {:?}", err))?,
                            }
                        } else {
                            return handle_err("Cannot assign null to non-nullable property");
                        }
                    }
                    Value::Int(i) => {
                        if (min.is_some() && *i < min.unwrap()) || (max.is_some() && *i > max.unwrap()) {
                            return handle_err("Value out of range");
                        }
                        match FBPutSlotInteger(fb, CString::new("value")?.as_ptr(), *i) {
                            PutSlotError::None => {}
                            err => handle_err(&format!("PutSlot error: {:?}", err))?,
                        }
                    }
                    _ => return handle_err("Property type and value type mismatch"),
                },
                Property::Float { nullable, min, max, .. } => match value {
                    Value::Null => {
                        if let Some(true) = nullable {
                            match FBPutSlotSymbol(fb, CString::new("value")?.as_ptr(), CString::new("nil")?.as_ptr()) {
                                PutSlotError::None => {}
                                err => handle_err(&format!("PutSlot error: {:?}", err))?,
                            }
                        } else {
                            return handle_err("Cannot assign null to non-nullable property");
                        }
                    }
                    Value::Float(f) => {
                        if (min.is_some() && *f < min.unwrap()) || (max.is_some() && *f > max.unwrap()) {
                            return handle_err("Value out of range");
                        }
                        match FBPutSlotFloat(fb, CString::new("value")?.as_ptr(), *f) {
                            PutSlotError::None => {}
                            err => handle_err(&format!("PutSlot error: {:?}", err))?,
                        }
                    }
                    _ => return handle_err("Property type and value type mismatch"),
                },
            }
            if let Some(t) = time {
                match FBPutSlotInteger(fb, CString::new(format!("time"))?.as_ptr(), t.timestamp()) {
                    PutSlotError::None => {}
                    err => handle_err(&format!("PutSlot error: {:?}", err))?,
                }
            }

            let fact = FBAssert(fb);
            if fact.is_null() {
                return handle_err(&format!("Assertion failed: {:?}", FBError(fb)));
            }
            self.facts.entry(class.name.clone()).or_default().entry(object.id.clone()).or_default().insert(property_name.to_string(), fact);

            FBDispose(fb);
            Ok(())
        }
    }

    fn update_prop(&mut self, object: &Object, class: &Class, property: &Property, property_name: &str, value: &Value, time: Option<&DateTime<Utc>>) -> Result<(), Box<dyn Error>> {
        unsafe {
            let fm = CreateFactModifier(self.env, *self.facts.get(&class.name).and_then(|objs| objs.get(&object.id)).and_then(|props| props.get(property_name)).ok_or("Property fact not found in knowledge base")?);
            if fm.is_null() {
                return Err("Failed to create FactBuilder".into());
            }
            let handle_err = |msg: &str| {
                FMDispose(fm);
                Err::<(), Box<dyn Error>>(msg.to_string().into())
            };
            match FMPutSlotSymbol(fm, CString::new("id")?.as_ptr(), CString::new(object.id.clone())?.as_ptr()) {
                PutSlotError::None => {}
                err => handle_err(&format!("PutSlot error: {:?}", err))?,
            }
            match property {
                Property::Bool { nullable, .. } => match value {
                    Value::Null => {
                        if let Some(true) = nullable {
                            match FMPutSlotSymbol(fm, CString::new("value")?.as_ptr(), CString::new("nil")?.as_ptr()) {
                                PutSlotError::None => {}
                                err => handle_err(&format!("PutSlot error: {:?}", err))?,
                            }
                        } else {
                            return handle_err("Cannot assign null to non-nullable property");
                        }
                    }
                    Value::Bool(b) => {
                        let symbol = if *b { "TRUE" } else { "FALSE" };
                        match FMPutSlotSymbol(fm, CString::new("value")?.as_ptr(), CString::new(symbol)?.as_ptr()) {
                            PutSlotError::None => {}
                            err => handle_err(&format!("PutSlot error: {:?}", err))?,
                        }
                    }
                    _ => return handle_err("Property type and value type mismatch"),
                },
                Property::Int { nullable, min, max, .. } => match value {
                    Value::Null => {
                        if let Some(true) = nullable {
                            match FMPutSlotSymbol(fm, CString::new("value")?.as_ptr(), CString::new("nil")?.as_ptr()) {
                                PutSlotError::None => {}
                                err => handle_err(&format!("PutSlot error: {:?}", err))?,
                            }
                        } else {
                            return handle_err("Cannot assign null to non-nullable property");
                        }
                    }
                    Value::Int(i) => {
                        if (min.is_some() && *i < min.unwrap()) || (max.is_some() && *i > max.unwrap()) {
                            return handle_err("Value out of range");
                        }
                        match FMPutSlotInteger(fm, CString::new("value")?.as_ptr(), *i) {
                            PutSlotError::None => {}
                            err => handle_err(&format!("PutSlot error: {:?}", err))?,
                        }
                    }
                    _ => return handle_err("Property type and value type mismatch"),
                },
                Property::Float { nullable, min, max, .. } => match value {
                    Value::Null => {
                        if let Some(true) = nullable {
                            match FMPutSlotSymbol(fm, CString::new("value")?.as_ptr(), CString::new("nil")?.as_ptr()) {
                                PutSlotError::None => {}
                                err => handle_err(&format!("PutSlot error: {:?}", err))?,
                            }
                        } else {
                            return handle_err("Cannot assign null to non-nullable property");
                        }
                    }
                    Value::Float(f) => {
                        if (min.is_some() && *f < min.unwrap()) || (max.is_some() && *f > max.unwrap()) {
                            return handle_err("Value out of range");
                        }
                        match FMPutSlotFloat(fm, CString::new("value")?.as_ptr(), *f) {
                            PutSlotError::None => {}
                            err => handle_err(&format!("PutSlot error: {:?}", err))?,
                        }
                    }
                    _ => return handle_err("Property type and value type mismatch"),
                },
            }
            if let Some(t) = time {
                match FMPutSlotInteger(fm, CString::new(format!("time"))?.as_ptr(), t.timestamp()) {
                    PutSlotError::None => {}
                    err => handle_err(&format!("PutSlot error: {:?}", err))?,
                }
            }

            let modified_fact = FMModify(fm);
            if modified_fact.is_null() {
                return handle_err(&format!("Modification failed: {:?}", FMError(fm)));
            }
            self.facts.get_mut(&class.name).and_then(|objs| objs.get_mut(&object.id)).and_then(|props| props.get_mut(property_name)).map(|f| *f = modified_fact);

            FMDispose(fm);
            Ok(())
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
    fn create_class(&mut self, class: Class) -> Result<&Class, Box<dyn Error>> {
        unsafe {
            match Build(self.env, CString::new(format!("(deftemplate {} (slot id (type SYMBOL)))", class.name))?.as_ptr()) {
                BuildError::None => {}
                err => return Err(format!("Build error: {:?}", err).into()),
            }
            if let Some(static_props) = &class.static_properties {
                for (name, prop) in static_props {
                    match Build(self.env, CString::new(prop_deftemplate(&class, name, prop, true))?.as_ptr()) {
                        BuildError::None => {}
                        err => return Err(format!("Build error: {:?}", err).into()),
                    }
                }
            }
            if let Some(dynamic_props) = &class.dynamic_properties {
                for (name, prop) in dynamic_props {
                    match Build(self.env, CString::new(prop_deftemplate(&class, name, prop, false))?.as_ptr()) {
                        BuildError::None => {}
                        err => return Err(format!("Build error: {:?}", err).into()),
                    }
                }
            }
            Ok(self.classes.entry(class.name.clone()).or_insert(class))
        }
    }

    fn create_object(&mut self, class: &Class, object: &Object) -> Result<(), Box<dyn Error>> {
        unsafe {
            let fb = CreateFactBuilder(self.env, CString::new(class.name.clone())?.as_ptr());
            if fb.is_null() {
                return Err("Failed to create FactBuilder".into());
            }

            match FBPutSlotSymbol(fb, CString::new("id")?.as_ptr(), CString::new(object.id.clone())?.as_ptr()) {
                PutSlotError::None => {}
                err => {
                    FBDispose(fb);
                    return Err(format!("PutSlot error: {:?}", err).into());
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

            if let Some(props) = class.static_properties.as_ref() {
                for (prop_name, prop) in props {
                    let value = object.properties.as_ref().and_then(|props| props.get(prop_name));
                    if let Some(v) = value {
                        self.set_prop(object, class, prop, prop_name, v, None)?;
                    } else {
                        let default_val = match prop {
                            Property::Bool { default: Some(v), .. } => Some(Value::Bool(*v)),
                            Property::Int { default: Some(v), .. } => Some(Value::Int(*v)),
                            Property::Float { default: Some(v), .. } => Some(Value::Float(*v)),
                            _ => None,
                        };
                        if let Some(v) = default_val {
                            self.set_prop(object, class, prop, prop_name, &v, None)?;
                        } else {
                            self.set_prop(object, class, prop, prop_name, &Value::Null, None)?;
                        }
                    }
                }
            }

            if let Some(props) = class.dynamic_properties.as_ref() {
                for (prop_name, prop) in props {
                    let value_time = object.values.as_ref().and_then(|vals| vals.get(prop_name));
                    if let Some((v, t)) = value_time {
                        self.set_prop(object, class, prop, prop_name, v, Some(t))?;
                    } else {
                        let default_val = match prop {
                            Property::Bool { default: Some(v), .. } => Some(Value::Bool(*v)),
                            Property::Int { default: Some(v), .. } => Some(Value::Int(*v)),
                            Property::Float { default: Some(v), .. } => Some(Value::Float(*v)),
                            _ => None,
                        };
                        if let Some(v) = default_val {
                            self.set_prop(object, class, prop, prop_name, &v, None)?;
                        } else {
                            self.set_prop(object, class, prop, prop_name, &Value::Null, None)?;
                        }
                    }
                }
            }

            Ok(())
        }
    }

    fn set_properties(&mut self, class: &Class, object: &Object, values: &HashMap<String, Value>) -> Result<(), Box<dyn Error>> {
        if let Some(props) = class.static_properties.as_ref() {
            for (prop_name, value) in values.iter() {
                if let Some(prop) = props.get(prop_name) {
                    if let Err(e) = self.update_prop(object, class, prop, prop_name, value, None) {
                        return Err(e);
                    }
                }
            }
        }

        Ok(())
    }

    fn add_data(&mut self, class: &Class, object: &Object, values: &HashMap<String, Value>, date_time: &DateTime<Utc>) -> Result<(), Box<dyn Error>> {
        if let Some(props) = class.dynamic_properties.as_ref() {
            for (prop_name, value) in values.iter() {
                if let Some(prop) = props.get(prop_name) {
                    self.update_prop(object, class, prop, prop_name, value, Some(date_time))?;
                }
            }
        }

        Ok(())
    }

    fn create_rule(&self, rule: &crate::Rule) -> Result<(), Box<dyn Error>> {
        unsafe {
            match Build(self.env, CString::new(rule.content.clone())?.as_ptr()) {
                BuildError::None => Ok(()),
                err => Err(format!("Build error: {:?}", err).into()),
            }
        }
    }

    fn set_data_callback<F>(&mut self, callback: F)
    where
        F: FnMut(&str, HashMap<String, Value>, DateTime<Utc>) + 'static,
    {
        self.data_callback = Box::new(callback);
    }

    fn run(&mut self) -> Result<(), Box<dyn Error>> {
        unsafe {
            let result = Run(self.env, -1);
            if result < 0 { Err(format!("Run error: {}", result).into()) } else { Ok(()) }
        }
    }
}

fn prop_deftemplate(class: &Class, name: &str, property: &Property, is_static: bool) -> String {
    let mut def = format!("(deftemplate {}_{} (slot id (type SYMBOL))", class.name, name);
    match property {
        Property::Bool { nullable, default } => {
            def.push_str(" (slot value (type SYMBOL) (allowed-symbols TRUE FALSE");
            if let Some(true) = nullable {
                def.push_str(" nil");
            }
            def.push(')');
            if let Some(def_val) = default {
                def.push_str(&format!(" (default {})", if *def_val { "TRUE" } else { "FALSE" }));
            } else if let Some(true) = nullable {
                def.push_str(" (default nil)");
            }
            def.push(')');
            if !is_static {
                def.push_str(" (slot time (type INTEGER))");
            }
            def.push(')');
            def
        }
        Property::Int { nullable, default, min, max } => {
            def.push_str(" (slot value (type INTEGER");
            if let Some(true) = nullable {
                def.push_str(" SYMBOL) (allowed-symbols nil");
            }
            def.push(')');
            if let Some(def_val) = default {
                def.push_str(&format!(" (default {})", def_val));
            } else if let Some(true) = nullable {
                def.push_str(" (default nil)");
            }
            if (min.is_some() || max.is_some()) && !nullable.unwrap_or(false) {
                let min_str = min.map(|v| v.to_string()).unwrap_or("?VARIABLE".to_string());
                let max_str = max.map(|v| v.to_string()).unwrap_or("?VARIABLE".to_string());
                def.push_str(&format!(" (range {} {})", min_str, max_str));
            }
            def.push(')');
            if !is_static {
                def.push_str(" (slot time (type INTEGER))");
            }
            def.push(')');
            def
        }
        Property::Float { nullable, default, min, max } => {
            def.push_str(" (slot value (type FLOAT");
            if let Some(true) = nullable {
                def.push_str(" SYMBOL) (allowed-symbols nil");
            }
            def.push(')');
            if let Some(def_val) = default {
                def.push_str(&format!(" (default {})", def_val));
            } else if let Some(true) = nullable {
                def.push_str(" (default nil)");
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
            if !is_static {
                def.push_str(" (slot time (type INTEGER))");
            }
            def.push(')');
            def
        }
    }
}

#[cfg(test)]
mod tests {
    use std::collections::HashSet;

    use super::*;

    fn create_test_kb() -> KnowledgeBase {
        KnowledgeBase::new()
    }

    fn create_test_class(kb: &mut KnowledgeBase) -> Class {
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

        kb.create_class(class).unwrap().clone()
    }

    #[test]
    fn test_lifecycle() {
        let kb = create_test_kb();
        assert!(!kb.env.is_null());
    }

    #[test]
    fn test_class_management() {
        let mut kb = create_test_kb();

        // Simple class
        let simple_class = Class {
            name: "SimpleClass".to_string(),
            parents: None,
            static_properties: None,
            dynamic_properties: None,
        };
        assert!(kb.create_class(simple_class).is_ok());

        // Complex class with properties
        create_test_class(&mut kb);
    }

    #[test]
    fn test_object_management() {
        let mut kb = create_test_kb();
        let class = create_test_class(&mut kb);

        let object = Object { id: "obj1".to_string(), classes: HashSet::new(), properties: None, values: None };

        assert!(kb.create_object(&class, &object).is_ok());
    }

    #[test]
    fn test_properties_management() {
        let mut kb = create_test_kb();
        let class = create_test_class(&mut kb);
        let object = Object { id: "obj1".to_string(), classes: HashSet::new(), properties: None, values: None };
        kb.create_object(&class, &object).unwrap();

        // Test set_properties (static)
        let mut props = HashMap::new();
        props.insert("is_valid".to_string(), Value::Bool(false));
        props.insert("score".to_string(), Value::Int(50));

        let result = kb.set_properties(&class, &object, &props);
        assert!(result.is_ok(), "Failed to set properties: {:?}", result.err());

        // Test invalid property value (out of range)
        let mut invalid_props = HashMap::new();
        invalid_props.insert("score".to_string(), Value::Int(150));
        let result = kb.set_properties(&class, &object, &invalid_props);
        assert!(result.is_err(), "Should have failed due to out of range value");
    }

    #[test]
    fn test_data_stream_management() {
        let mut kb = create_test_kb();
        let class = create_test_class(&mut kb);
        let object = Object { id: "obj1".to_string(), classes: HashSet::new(), properties: None, values: None };
        kb.create_object(&class, &object).unwrap();

        // Test add_data (dynamic)
        let mut values = HashMap::new();
        values.insert("temperature".to_string(), Value::Float(37.5));

        let now = Utc::now();
        let result = kb.add_data(&class, &object, &values, &now);
        assert!(result.is_ok(), "Failed to add data: {:?}", result.err());

        // Test invalid data (wrong type)
        let mut invalid_values = HashMap::new();
        invalid_values.insert("temperature".to_string(), Value::Bool(true));

        let result = kb.add_data(&class, &object, &invalid_values, &now);
        assert!(result.is_err(), "Should have failed due to type mismatch");
    }

    #[test]
    fn test_rules_and_execution() {
        let mut kb = create_test_kb();

        let rule = crate::Rule {
            name: "TestRule".to_string(),
            content: "(defrule test_rule => (printout t \"Hello World\" crlf))".to_string(),
        };

        assert!(kb.create_rule(&rule).is_ok());
        assert!(kb.run().is_ok());
    }

    #[test]
    fn test_udf_integration() {
        let kb = create_test_kb();

        unsafe extern "C" fn test_udf(_env: *mut Environment, _udfc: *mut UDFContext, _out: *mut UDFValue) {
            // Function logic placeholder
        }

        let result = kb.add_udf("test_function", "v", 0, 0, "", test_udf, "test_udf");

        assert!(result.is_ok(), "Failed to add UDF: {:?}", result.err());
    }
}

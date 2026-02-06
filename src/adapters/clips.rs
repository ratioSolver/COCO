use crate::{Class, CoCoEvent, KnowledgeBase, Object, Property, Value};
use chrono::{DateTime, Utc};
use std::{
    collections::HashMap,
    error::Error,
    ffi::{CString, c_char, c_double, c_long, c_longlong, c_ushort},
    marker::{PhantomData, PhantomPinned},
    os::raw::c_void,
    sync::RwLock,
};
use tokio::sync::broadcast;

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
    pub type_code: CLIPSTypeCode,
}

#[repr(C)]
#[repr(C)]
pub struct CLIPSLexeme {
    pub header: TypeHeader,
    pub contents: *const c_char,
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
pub struct Multifield {
    pub header: TypeHeader,
    pub length: usize,
    pub contents: *mut UDFValue,
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

#[repr(u16)]
#[allow(dead_code)]
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum CLIPSTypeCode {
    Float,
    Integer,
    Symbol,
    String,
    Multifield,
    ExternalAddress,
    FactAddress,
    InstanceAddress,
    InstanceName,
    Void,
}

#[repr(C)]
#[allow(dead_code)]
#[derive(Debug, Clone, Copy)]
pub enum CLIPSType {
    FloatBit = 1 << 0,
    IntegerBit = 1 << 1,
    SymbolBit = 1 << 2,
    StringBit = 1 << 3,
    MultifieldBit = 1 << 4,
    ExternalAddressBit = 1 << 5,
    FactAddressBit = 1 << 6,
    InstanceAddressBit = 1 << 7,
    InstanceNameBit = 1 << 8,
    VoidBit = 1 << 9,
    BooleanBit = 1 << 10,
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
    unsafe fn UDFFirstArgument(udfc: *mut UDFContext, expected_type: CLIPSType, out: *mut UDFValue) -> bool;
    unsafe fn UDFHasNextArgument(udfc: *mut UDFContext) -> bool;
    unsafe fn UDFNextArgument(udfc: *mut UDFContext, expected_type: CLIPSType, out: *mut UDFValue) -> bool;
    unsafe fn Run(env: *mut Environment, run_limit: c_long) -> c_long;
}

pub struct CLIPSKnowledgeBase {
    sender: broadcast::Sender<CoCoEvent>,
    env: *mut Environment,
    instances: RwLock<HashMap<String, HashMap<String, *mut Fact>>>,              // class -> object -> fact
    facts: RwLock<HashMap<String, HashMap<String, HashMap<String, *mut Fact>>>>, // class -> object -> property -> fact
}

unsafe impl Send for CLIPSKnowledgeBase {}
unsafe impl Sync for CLIPSKnowledgeBase {}

impl CLIPSKnowledgeBase {
    pub fn new(sender: broadcast::Sender<CoCoEvent>) -> Self {
        unsafe {
            CLIPSKnowledgeBase {
                sender,
                env: CreateEnvironment(),
                instances: RwLock::new(HashMap::new()),
                facts: RwLock::new(HashMap::new()),
            }
        }
    }

    pub fn init(&self) {
        unsafe {
            AddUDF(self.env, CString::new("add-values").unwrap().as_ptr(), CString::new("v").unwrap().as_ptr(), 3, 4, CString::new("ymml").unwrap().as_ptr(), add_values, CString::new("add-values").unwrap().as_ptr(), self as *const _ as *mut c_void);
            AddUDF(self.env, CString::new("add-class").unwrap().as_ptr(), CString::new("v").unwrap().as_ptr(), 2, 2, CString::new("yy").unwrap().as_ptr(), add_class, CString::new("add-class").unwrap().as_ptr(), self as *const _ as *mut c_void);
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

    fn set_prop(&self, object: &Object, class: &Class, property: &Property, property_name: &str, value: &Value, time: Option<&DateTime<Utc>>) -> Result<(), Box<dyn Error>> {
        unsafe {
            let fb = CreateFactBuilder(self.env, CString::new(format!("{}_{}", class.name, property_name))?.as_ptr());
            if fb.is_null() {
                return Err("Failed to create FactBuilder".into());
            }
            let handle_err = |msg: &str| {
                FBDispose(fb);
                Err(msg.to_string().into())
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
                Property::String { nullable, .. } => match value {
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
                    Value::String(s) => match FBPutSlotString(fb, CString::new("value")?.as_ptr(), CString::new(s.clone())?.as_ptr()) {
                        PutSlotError::None => {}
                        err => handle_err(&format!("PutSlot error: {:?}", err))?,
                    },
                    _ => return handle_err("Property type and value type mismatch"),
                },
                Property::Symbol { nullable, allowed_values, .. } => match value {
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
                    Value::Symbol(s) => {
                        if let Some(allowed) = allowed_values {
                            if !allowed.contains(s) {
                                return handle_err("Value not in allowed values");
                            }
                        }
                        match FBPutSlotSymbol(fb, CString::new("value")?.as_ptr(), CString::new(s.clone())?.as_ptr()) {
                            PutSlotError::None => {}
                            err => handle_err(&format!("PutSlot error: {:?}", err))?,
                        }
                    }
                    _ => return handle_err("Property type and value type mismatch"),
                },
                Property::Object { nullable, class, .. } => match value {
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
                    Value::Object(o) => {
                        if !self.instances.read().unwrap().get(class).map_or(false, |objs| objs.contains_key(o)) {
                            return handle_err("Object of specified class not found");
                        }
                        match FBPutSlotSymbol(fb, CString::new("value")?.as_ptr(), CString::new(o.clone())?.as_ptr()) {
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
            self.facts.write().unwrap().entry(class.name.clone()).or_default().entry(object.id.clone()).or_default().insert(property_name.to_string(), fact);

            FBDispose(fb);
            Ok(())
        }
    }

    fn update_prop(&self, object: &Object, class: &Class, property: &Property, property_name: &str, value: &Value, time: Option<&DateTime<Utc>>) -> Result<(), Box<dyn Error>> {
        unsafe {
            let fm = CreateFactModifier(self.env, *self.facts.read().unwrap().get(&class.name).and_then(|objs| objs.get(&object.id)).and_then(|props| props.get(property_name)).ok_or("Property fact not found in knowledge base")?);
            if fm.is_null() {
                return Err("Failed to create FactBuilder".into());
            }
            let handle_err = |msg: &str| {
                FMDispose(fm);
                Err(msg.to_string().into())
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
                Property::String { nullable, .. } => match value {
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
                    Value::String(s) => match FMPutSlotString(fm, CString::new("value")?.as_ptr(), CString::new(s.clone())?.as_ptr()) {
                        PutSlotError::None => {}
                        err => handle_err(&format!("PutSlot error: {:?}", err))?,
                    },
                    _ => return handle_err("Property type and value type mismatch"),
                },
                Property::Symbol { nullable, allowed_values, .. } => match value {
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
                    Value::Symbol(s) => {
                        if let Some(allowed) = allowed_values {
                            if !allowed.contains(s) {
                                return handle_err("Value not in allowed values");
                            }
                        }
                        match FMPutSlotSymbol(fm, CString::new("value")?.as_ptr(), CString::new(s.clone())?.as_ptr()) {
                            PutSlotError::None => {}
                            err => handle_err(&format!("PutSlot error: {:?}", err))?,
                        }
                    }
                    _ => return handle_err("Property type and value type mismatch"),
                },
                Property::Object { nullable, class, .. } => match value {
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
                    Value::Object(o) => {
                        if !self.instances.read().unwrap().get(class).map_or(false, |objs| objs.contains_key(o)) {
                            return handle_err("Object of specified class not found");
                        }
                        match FMPutSlotSymbol(fm, CString::new("value")?.as_ptr(), CString::new(o.clone())?.as_ptr()) {
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
            self.facts.write().unwrap().get_mut(&class.name).and_then(|objs| objs.get_mut(&object.id)).and_then(|props| props.get_mut(property_name)).map(|f| *f = modified_fact);

            FMDispose(fm);
            Ok(())
        }
    }
}

impl KnowledgeBase for CLIPSKnowledgeBase {
    fn get_event_sender(&self) -> broadcast::Sender<CoCoEvent> {
        self.sender.clone()
    }

    fn create_class(&self, class: &Class) -> Result<(), Box<dyn Error>> {
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
            Ok(())
        }
    }

    fn create_object(&self, class: &Class, object: &Object) -> Result<(), Box<dyn Error>> {
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

            self.instances.write().unwrap().entry(class.name.clone()).or_default().insert(object.id.clone(), fact);

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

    fn set_properties(&self, class: &Class, object: &Object, values: &HashMap<String, Value>) -> Result<(), Box<dyn Error>> {
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

    fn add_data(&self, class: &Class, object: &Object, values: &HashMap<String, Value>, date_time: &DateTime<Utc>) -> Result<(), Box<dyn Error>> {
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
        Property::String { nullable, default } => {
            def.push_str(" (slot value (type STRING");
            if let Some(true) = nullable {
                def.push_str(" SYMBOL) (allowed-symbols nil");
            }
            def.push(')');
            if let Some(def_val) = default {
                def.push_str(&format!(" (default \"{}\")", def_val));
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
        Property::Symbol { nullable, default, allowed_values } => {
            def.push_str(" (slot value (type SYMBOL)");
            if let Some(allowed) = allowed_values {
                def.push_str(" (allowed-symbols");
                if let Some(true) = nullable {
                    def.push_str(" nil");
                }
                for v in allowed {
                    def.push_str(&format!(" {}", v));
                }
                def.push(')');
            } else if let Some(true) = nullable {
                def.push_str(" (allowed-symbols nil)");
            }

            if let Some(def_val) = default {
                def.push_str(&format!(" (default {})", def_val));
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
        Property::Object { nullable, default, .. } => {
            def.push_str(" (slot value (type SYMBOL)");
            if let Some(def_val) = default {
                def.push_str(&format!(" (default {})", def_val));
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
    }
}

unsafe extern "C" fn add_values(_env: *mut Environment, udfc: *mut UDFContext, _out: *mut UDFValue) {
    unsafe {
        let kb = &*((*udfc).context as *mut CLIPSKnowledgeBase);
        let mut object_id = std::mem::MaybeUninit::<UDFValue>::uninit();
        if !UDFFirstArgument(udfc, CLIPSType::SymbolBit, object_id.as_mut_ptr()) {
            return;
        }
        let object_id = object_id.assume_init();
        assert!(object_id.value.header.as_ref().unwrap().type_code == CLIPSTypeCode::Symbol);
        let object_id = std::ffi::CStr::from_ptr((*object_id.value.lexeme_value).contents).to_str().unwrap();

        let mut pars = std::mem::MaybeUninit::<UDFValue>::uninit();
        if !UDFNextArgument(udfc, CLIPSType::MultifieldBit, pars.as_mut_ptr()) {
            return;
        }
        let pars = pars.assume_init();
        let mut vars = std::mem::MaybeUninit::<UDFValue>::uninit();
        if !UDFNextArgument(udfc, CLIPSType::MultifieldBit, vars.as_mut_ptr()) {
            return;
        }
        let vars = vars.assume_init();

        let time = if UDFHasNextArgument(udfc) {
            let mut time_val = std::mem::MaybeUninit::<UDFValue>::uninit();
            if !UDFNextArgument(udfc, CLIPSType::IntegerBit, time_val.as_mut_ptr()) {
                return;
            }
            let time_val = time_val.assume_init();
            DateTime::<Utc>::from_timestamp(time_val.value.integer_value as i64, 0).unwrap()
        } else {
            Utc::now()
        };

        assert!((*pars.value.multifield_value).length == (*vars.value.multifield_value).length);
        let length = (*pars.value.multifield_value).length;
        let pars_contents = (*pars.value.multifield_value).contents;
        let vars_contents = (*vars.value.multifield_value).contents;

        let mut values = HashMap::new();
        for i in 0..length {
            let par = &*pars_contents.add(i);
            assert!(par.value.header.as_ref().unwrap().type_code == CLIPSTypeCode::Symbol);
            let par_name = std::ffi::CStr::from_ptr((*par.value.lexeme_value).contents).to_str().unwrap();
            let val = &*vars_contents.add(i);
            match par.value.header.as_ref().unwrap().type_code {
                CLIPSTypeCode::Symbol => {
                    let val_str = std::ffi::CStr::from_ptr((*val.value.lexeme_value).contents).to_str().unwrap();
                    match val_str {
                        "TRUE" => {
                            values.insert(par_name.to_string(), (Value::Bool(true), time));
                        }
                        "FALSE" => {
                            values.insert(par_name.to_string(), (Value::Bool(false), time));
                        }
                        "nil" => {
                            values.insert(par_name.to_string(), (Value::Null, time));
                        }
                        _ => {}
                    }
                }
                CLIPSTypeCode::Integer => {
                    values.insert(par_name.to_string(), (Value::Int((*val.value.integer_value).contents), time));
                }
                CLIPSTypeCode::Float => {
                    values.insert(par_name.to_string(), (Value::Float((*val.value.float_value).contents), time));
                }
                _ => {}
            }
        }

        let sender = &kb.sender;
        let _ = sender.send(CoCoEvent::AddedValues(object_id.to_string(), values));
    }
}

unsafe extern "C" fn add_class(_env: *mut Environment, udfc: *mut UDFContext, _out: *mut UDFValue) {
    unsafe {
        let kb = &*((*udfc).context as *mut CLIPSKnowledgeBase);
        let mut object_id = std::mem::MaybeUninit::<UDFValue>::uninit();
        if !UDFFirstArgument(udfc, CLIPSType::SymbolBit, object_id.as_mut_ptr()) {
            return;
        }
        let object_id = object_id.assume_init();
        assert!(object_id.value.header.as_ref().unwrap().type_code == CLIPSTypeCode::Symbol);
        let object_id = std::ffi::CStr::from_ptr((*object_id.value.lexeme_value).contents).to_str().unwrap();

        let mut class_name = std::mem::MaybeUninit::<UDFValue>::uninit();
        if !UDFNextArgument(udfc, CLIPSType::SymbolBit, class_name.as_mut_ptr()) {
            return;
        }
        let class_name = class_name.assume_init();
        assert!(class_name.value.header.as_ref().unwrap().type_code == CLIPSTypeCode::Symbol);
        let class_name = std::ffi::CStr::from_ptr((*class_name.value.lexeme_value).contents).to_str().unwrap();

        let sender = &kb.sender;
        let _ = sender.send(CoCoEvent::AddedClass(object_id.to_string(), class_name.to_string()));
    }
}

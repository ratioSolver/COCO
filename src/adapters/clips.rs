use crate::{Class, CoCoEvent, KnowledgeBase, Object, Property, Rule, Value};
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
struct MultifieldBuilder {
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
    pub environment: *mut Environment,
    pub context: *mut c_void,
    pub the_function: *mut c_void,
    pub last_position: u32,
    pub last_arg: *mut c_void,
    pub return_value: *mut UDFValue,
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
    unsafe fn FBPutSlotInteger(fb: *mut FactBuilder, slot_name: *const c_char, value: c_longlong) -> PutSlotError;
    unsafe fn FBPutSlotFloat(fb: *mut FactBuilder, slot_name: *const c_char, value: c_double) -> PutSlotError;
    unsafe fn FBPutSlotSymbol(fb: *mut FactBuilder, slot_name: *const c_char, value: *const c_char) -> PutSlotError;
    unsafe fn FBPutSlotString(fb: *mut FactBuilder, slot_name: *const c_char, value: *const c_char) -> PutSlotError;
    unsafe fn FBPutSlotMultifield(fb: *mut FactBuilder, slot_name: *const c_char, mf: *mut Multifield) -> PutSlotError;

    unsafe fn CreateFactModifier(env: *mut Environment, fact: *mut Fact) -> *mut FactModifier;
    unsafe fn FMModify(fm: *mut FactModifier) -> *mut Fact;
    unsafe fn FMDispose(fm: *mut FactModifier);
    unsafe fn FMError(fm: *mut FactModifier) -> FactModifierError;
    unsafe fn FMPutSlotInteger(fm: *mut FactModifier, slot_name: *const c_char, value: c_longlong) -> PutSlotError;
    unsafe fn FMPutSlotFloat(fm: *mut FactModifier, slot_name: *const c_char, value: c_double) -> PutSlotError;
    unsafe fn FMPutSlotSymbol(fm: *mut FactModifier, slot_name: *const c_char, value: *const c_char) -> PutSlotError;
    unsafe fn FMPutSlotString(fm: *mut FactModifier, slot_name: *const c_char, value: *const c_char) -> PutSlotError;
    unsafe fn FMPutSlotMultifield(fm: *mut FactModifier, slot_name: *const c_char, mf: *mut Multifield) -> PutSlotError;

    unsafe fn CreateMultifieldBuilder(env: *mut Environment, capacity: usize) -> *mut MultifieldBuilder;
    unsafe fn MBCreate(mb: *mut MultifieldBuilder) -> *mut Multifield;
    unsafe fn MBDispose(mb: *mut MultifieldBuilder);
    unsafe fn MBAppendInteger(mb: *mut MultifieldBuilder, value: c_longlong);
    unsafe fn MBAppendFloat(mb: *mut MultifieldBuilder, value: c_double);
    unsafe fn MBAppendSymbol(mb: *mut MultifieldBuilder, value: *const c_char);
    unsafe fn MBAppendString(mb: *mut MultifieldBuilder, value: *const c_char);

    unsafe fn AddUDF(env: *mut Environment, name: *const c_char, return_types: *const c_char, min_args: c_ushort, max_args: c_ushort, arg_types: *const c_char, function_ptr: UserDefinedFunction, r_name: *const c_char, context: *mut c_void) -> AddUDFError;
    unsafe fn UDFFirstArgument(udfc: *mut UDFContext, expected_type: CLIPSType, out: *mut UDFValue) -> bool;
    unsafe fn UDFNextArgument(udfc: *mut UDFContext, expected_type: CLIPSType, out: *mut UDFValue) -> bool;
    unsafe fn Run(env: *mut Environment, run_limit: c_long) -> c_long;
}

pub struct CLIPSKnowledgeBase {
    sender: broadcast::Sender<CoCoEvent>,
    env: *mut Environment,
    classes: RwLock<HashMap<String, Class>>,
    objects: RwLock<HashMap<String, Object>>,
    rules: RwLock<HashMap<String, Rule>>,
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
                classes: RwLock::new(HashMap::new()),
                objects: RwLock::new(HashMap::new()),
                rules: RwLock::new(HashMap::new()),
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
            match FBPutSlotSymbol(fb, CString::new("id")?.as_ptr(), CString::new(object.id.as_ref().unwrap().clone())?.as_ptr()) {
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
                        if let Some(allowed) = allowed_values
                            && !allowed.contains(s)
                        {
                            return handle_err("Value not in allowed values");
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
                        if !self.instances.read().unwrap().get(class).is_some_and(|objs| objs.contains_key(o)) {
                            return handle_err("Object of specified class not found");
                        }
                        match FBPutSlotSymbol(fb, CString::new("value")?.as_ptr(), CString::new(o.clone())?.as_ptr()) {
                            PutSlotError::None => {}
                            err => handle_err(&format!("PutSlot error: {:?}", err))?,
                        }
                    }
                    _ => return handle_err("Property type and value type mismatch"),
                },
                Property::BoolArray { default } => match value {
                    Value::Null => {
                        if let Some(default) = default {
                            let mb = CreateMultifieldBuilder(self.env, default.len());
                            if mb.is_null() {
                                return handle_err("Failed to create MultifieldBuilder");
                            }
                            for b in default {
                                MBAppendSymbol(mb, CString::new(if *b { "TRUE" } else { "FALSE" })?.as_ptr());
                            }
                            let mf = MBCreate(mb);
                            MBDispose(mb);
                            match FBPutSlotMultifield(fb, CString::new("value")?.as_ptr(), mf) {
                                PutSlotError::None => {}
                                err => handle_err(&format!("PutSlot error: {:?}", err))?,
                            }
                        } else {
                            return handle_err("Cannot assign null to non-nullable property");
                        }
                    }
                    Value::BoolArray(arr) => {
                        let mb = CreateMultifieldBuilder(self.env, arr.len());
                        if mb.is_null() {
                            return handle_err("Failed to create MultifieldBuilder");
                        }
                        for b in arr {
                            MBAppendSymbol(mb, CString::new(if *b { "TRUE" } else { "FALSE" })?.as_ptr());
                        }
                        let mf = MBCreate(mb);
                        MBDispose(mb);
                        match FBPutSlotMultifield(fb, CString::new("value")?.as_ptr(), mf) {
                            PutSlotError::None => {}
                            err => handle_err(&format!("PutSlot error: {:?}", err))?,
                        }
                    }
                    _ => return handle_err("Property type and value type mismatch"),
                },
                Property::IntArray { default, min, max } => match value {
                    Value::Null => {
                        if let Some(default) = default {
                            let mb = CreateMultifieldBuilder(self.env, default.len());
                            if mb.is_null() {
                                return handle_err("Failed to create MultifieldBuilder");
                            }
                            for i in default {
                                if (min.is_some() && *i < min.unwrap()) || (max.is_some() && *i > max.unwrap()) {
                                    MBDispose(mb);
                                    return handle_err("Value out of range");
                                }
                                MBAppendInteger(mb, *i);
                            }
                            let mf = MBCreate(mb);
                            MBDispose(mb);
                            match FBPutSlotMultifield(fb, CString::new("value")?.as_ptr(), mf) {
                                PutSlotError::None => {}
                                err => handle_err(&format!("PutSlot error: {:?}", err))?,
                            }
                        } else {
                            return handle_err("Cannot assign null to non-nullable property");
                        }
                    }
                    Value::IntArray(arr) => {
                        let mb = CreateMultifieldBuilder(self.env, arr.len());
                        if mb.is_null() {
                            return handle_err("Failed to create MultifieldBuilder");
                        }
                        for i in arr {
                            if (min.is_some() && *i < min.unwrap()) || (max.is_some() && *i > max.unwrap()) {
                                MBDispose(mb);
                                return handle_err("Value out of range");
                            }
                            MBAppendInteger(mb, *i);
                        }
                        let mf = MBCreate(mb);
                        MBDispose(mb);
                        match FBPutSlotMultifield(fb, CString::new("value")?.as_ptr(), mf) {
                            PutSlotError::None => {}
                            err => handle_err(&format!("PutSlot error: {:?}", err))?,
                        }
                    }
                    _ => return handle_err("Property type and value type mismatch"),
                },
                Property::FloatArray { default, min, max } => match value {
                    Value::Null => {
                        if let Some(default) = default {
                            let mb = CreateMultifieldBuilder(self.env, default.len());
                            if mb.is_null() {
                                return handle_err("Failed to create MultifieldBuilder");
                            }
                            for f in default {
                                if (min.is_some() && *f < min.unwrap()) || (max.is_some() && *f > max.unwrap()) {
                                    MBDispose(mb);
                                    return handle_err("Value out of range");
                                }
                                MBAppendFloat(mb, *f);
                            }
                            let mf = MBCreate(mb);
                            MBDispose(mb);
                            match FBPutSlotMultifield(fb, CString::new("value")?.as_ptr(), mf) {
                                PutSlotError::None => {}
                                err => handle_err(&format!("PutSlot error: {:?}", err))?,
                            }
                        } else {
                            return handle_err("Cannot assign null to non-nullable property");
                        }
                    }
                    Value::FloatArray(arr) => {
                        let mb = CreateMultifieldBuilder(self.env, arr.len());
                        if mb.is_null() {
                            return handle_err("Failed to create MultifieldBuilder");
                        }
                        for f in arr {
                            if (min.is_some() && *f < min.unwrap()) || (max.is_some() && *f > max.unwrap()) {
                                MBDispose(mb);
                                return handle_err("Value out of range");
                            }
                            MBAppendFloat(mb, *f);
                        }
                        let mf = MBCreate(mb);
                        MBDispose(mb);
                        match FBPutSlotMultifield(fb, CString::new("value")?.as_ptr(), mf) {
                            PutSlotError::None => {}
                            err => handle_err(&format!("PutSlot error: {:?}", err))?,
                        }
                    }
                    _ => return handle_err("Property type and value type mismatch"),
                },
                Property::StringArray { default } => match value {
                    Value::Null => {
                        if let Some(default) = default {
                            let mb = CreateMultifieldBuilder(self.env, default.len());
                            if mb.is_null() {
                                return handle_err("Failed to create MultifieldBuilder");
                            }
                            for s in default {
                                MBAppendString(mb, CString::new(s.clone())?.as_ptr());
                            }
                            let mf = MBCreate(mb);
                            MBDispose(mb);
                            match FBPutSlotMultifield(fb, CString::new("value")?.as_ptr(), mf) {
                                PutSlotError::None => {}
                                err => handle_err(&format!("PutSlot error: {:?}", err))?,
                            }
                        } else {
                            return handle_err("Cannot assign null to non-nullable property");
                        }
                    }
                    Value::StringArray(arr) => {
                        let mb = CreateMultifieldBuilder(self.env, arr.len());
                        if mb.is_null() {
                            return handle_err("Failed to create MultifieldBuilder");
                        }
                        for s in arr {
                            MBAppendString(mb, CString::new(s.clone())?.as_ptr());
                        }
                        let mf = MBCreate(mb);
                        MBDispose(mb);
                        match FBPutSlotMultifield(fb, CString::new("value")?.as_ptr(), mf) {
                            PutSlotError::None => {}
                            err => handle_err(&format!("PutSlot error: {:?}", err))?,
                        }
                    }
                    _ => return handle_err("Property type and value type mismatch"),
                },
                Property::SymbolArray { default, allowed_values } => match value {
                    Value::Null => {
                        if let Some(default) = default {
                            let mb = CreateMultifieldBuilder(self.env, default.len());
                            if mb.is_null() {
                                return handle_err("Failed to create MultifieldBuilder");
                            }
                            for s in default {
                                if let Some(allowed) = allowed_values
                                    && !allowed.contains(s)
                                {
                                    MBDispose(mb);
                                    return handle_err("Value not in allowed values");
                                }
                                MBAppendSymbol(mb, CString::new(s.clone())?.as_ptr());
                            }
                            let mf = MBCreate(mb);
                            MBDispose(mb);
                            match FBPutSlotMultifield(fb, CString::new("value")?.as_ptr(), mf) {
                                PutSlotError::None => {}
                                err => handle_err(&format!("PutSlot error: {:?}", err))?,
                            }
                        } else {
                            return handle_err("Cannot assign null to non-nullable property");
                        }
                    }
                    Value::StringArray(arr) => {
                        let mb = CreateMultifieldBuilder(self.env, arr.len());
                        if mb.is_null() {
                            return handle_err("Failed to create MultifieldBuilder");
                        }
                        for s in arr {
                            if let Some(allowed) = allowed_values
                                && !allowed.contains(s)
                            {
                                MBDispose(mb);
                                return handle_err("Value not in allowed values");
                            }
                            MBAppendSymbol(mb, CString::new(s.clone())?.as_ptr());
                        }
                        let mf = MBCreate(mb);
                        MBDispose(mb);
                        match FBPutSlotMultifield(fb, CString::new("value")?.as_ptr(), mf) {
                            PutSlotError::None => {}
                            err => handle_err(&format!("PutSlot error: {:?}", err))?,
                        }
                    }
                    _ => return handle_err("Property type and value type mismatch"),
                },
                Property::ObjectArray { default, class, .. } => match value {
                    Value::Null => {
                        if let Some(default) = default {
                            let mb = CreateMultifieldBuilder(self.env, default.len());
                            if mb.is_null() {
                                return handle_err("Failed to create MultifieldBuilder");
                            }
                            for o in default {
                                if !self.instances.read().unwrap().get(class).is_some_and(|objs| objs.contains_key(o)) {
                                    MBDispose(mb);
                                    return handle_err("Object of specified class not found");
                                }
                                MBAppendSymbol(mb, CString::new(o.clone())?.as_ptr());
                            }
                            let mf = MBCreate(mb);
                            MBDispose(mb);
                            match FBPutSlotMultifield(fb, CString::new("value")?.as_ptr(), mf) {
                                PutSlotError::None => {}
                                err => handle_err(&format!("PutSlot error: {:?}", err))?,
                            }
                        } else {
                            return handle_err("Cannot assign null to non-nullable property");
                        }
                    }
                    Value::StringArray(arr) => {
                        let mb = CreateMultifieldBuilder(self.env, arr.len());
                        if mb.is_null() {
                            return handle_err("Failed to create MultifieldBuilder");
                        }
                        for o in arr {
                            if !self.instances.read().unwrap().get(class).is_some_and(|objs| objs.contains_key(o)) {
                                MBDispose(mb);
                                return handle_err("Object of specified class not found");
                            }
                            MBAppendSymbol(mb, CString::new(o.clone())?.as_ptr());
                        }
                        let mf = MBCreate(mb);
                        MBDispose(mb);
                        match FBPutSlotMultifield(fb, CString::new("value")?.as_ptr(), mf) {
                            PutSlotError::None => {}
                            err => handle_err(&format!("PutSlot error: {:?}", err))?,
                        }
                    }
                    _ => return handle_err("Property type and value type mismatch"),
                },
            }
            if let Some(t) = time {
                match FBPutSlotInteger(fb, CString::new("time".to_string())?.as_ptr(), t.timestamp()) {
                    PutSlotError::None => {}
                    err => handle_err(&format!("PutSlot error: {:?}", err))?,
                }
            }

            let fact = FBAssert(fb);
            if fact.is_null() {
                return handle_err(&format!("Assertion failed: {:?}", FBError(fb)));
            }
            self.facts.write().unwrap().entry(class.name.clone()).or_default().entry(object.id.as_ref().unwrap().clone()).or_default().insert(property_name.to_string(), fact);

            FBDispose(fb);
            Ok(())
        }
    }

    fn update_prop(&self, object: &Object, class: &Class, property: &Property, property_name: &str, value: &Value, time: Option<&DateTime<Utc>>) -> Result<(), Box<dyn Error>> {
        unsafe {
            let fm = CreateFactModifier(self.env, *self.facts.read().unwrap().get(&class.name).and_then(|objs| objs.get(object.id.as_ref().unwrap())).and_then(|props| props.get(property_name)).ok_or("Property fact not found in knowledge base")?);
            if fm.is_null() {
                return Err("Failed to create FactBuilder".into());
            }
            let handle_err = |msg: &str| {
                FMDispose(fm);
                Err(msg.to_string().into())
            };
            match FMPutSlotSymbol(fm, CString::new("id")?.as_ptr(), CString::new(object.id.as_ref().unwrap().clone())?.as_ptr()) {
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
                        if let Some(allowed) = allowed_values
                            && !allowed.contains(s)
                        {
                            return handle_err("Value not in allowed values");
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
                        if !self.instances.read().unwrap().get(class).is_some_and(|objs| objs.contains_key(o)) {
                            return handle_err("Object of specified class not found");
                        }
                        match FMPutSlotSymbol(fm, CString::new("value")?.as_ptr(), CString::new(o.clone())?.as_ptr()) {
                            PutSlotError::None => {}
                            err => handle_err(&format!("PutSlot error: {:?}", err))?,
                        }
                    }
                    _ => return handle_err("Property type and value type mismatch"),
                },
                Property::BoolArray { .. } => match value {
                    Value::Null => {
                        let mb = CreateMultifieldBuilder(self.env, 0);
                        if mb.is_null() {
                            return handle_err("Failed to create MultifieldBuilder");
                        }
                        let mf = MBCreate(mb);
                        MBDispose(mb);
                        match FMPutSlotMultifield(fm, CString::new("value")?.as_ptr(), mf) {
                            PutSlotError::None => {}
                            err => handle_err(&format!("PutSlot error: {:?}", err))?,
                        }
                    }
                    Value::BoolArray(arr) => {
                        let mb = CreateMultifieldBuilder(self.env, arr.len());
                        if mb.is_null() {
                            return handle_err("Failed to create MultifieldBuilder");
                        }
                        for b in arr {
                            MBAppendSymbol(mb, CString::new(if *b { "TRUE" } else { "FALSE" })?.as_ptr());
                        }
                        let mf = MBCreate(mb);
                        MBDispose(mb);
                        match FMPutSlotMultifield(fm, CString::new("value")?.as_ptr(), mf) {
                            PutSlotError::None => {}
                            err => handle_err(&format!("PutSlot error: {:?}", err))?,
                        }
                    }
                    _ => return handle_err("Property type and value type mismatch"),
                },
                Property::IntArray { min, max, .. } => match value {
                    Value::Null => {
                        let mb = CreateMultifieldBuilder(self.env, 0);
                        if mb.is_null() {
                            return handle_err("Failed to create MultifieldBuilder");
                        }
                        let mf = MBCreate(mb);
                        MBDispose(mb);
                        match FMPutSlotMultifield(fm, CString::new("value")?.as_ptr(), mf) {
                            PutSlotError::None => {}
                            err => handle_err(&format!("PutSlot error: {:?}", err))?,
                        }
                    }
                    Value::IntArray(arr) => {
                        let mb = CreateMultifieldBuilder(self.env, arr.len());
                        if mb.is_null() {
                            return handle_err("Failed to create MultifieldBuilder");
                        }
                        for i in arr {
                            if (min.is_some() && *i < min.unwrap()) || (max.is_some() && *i > max.unwrap()) {
                                MBDispose(mb);
                                return handle_err("Value out of range");
                            }
                            MBAppendInteger(mb, *i);
                        }
                        let mf = MBCreate(mb);
                        MBDispose(mb);
                        match FMPutSlotMultifield(fm, CString::new("value")?.as_ptr(), mf) {
                            PutSlotError::None => {}
                            err => handle_err(&format!("PutSlot error: {:?}", err))?,
                        }
                    }
                    _ => return handle_err("Property type and value type mismatch"),
                },
                Property::FloatArray { min, max, .. } => match value {
                    Value::Null => {
                        let mb = CreateMultifieldBuilder(self.env, 0);
                        if mb.is_null() {
                            return handle_err("Failed to create MultifieldBuilder");
                        }
                        let mf = MBCreate(mb);
                        MBDispose(mb);
                        match FMPutSlotMultifield(fm, CString::new("value")?.as_ptr(), mf) {
                            PutSlotError::None => {}
                            err => handle_err(&format!("PutSlot error: {:?}", err))?,
                        }
                    }
                    Value::FloatArray(arr) => {
                        let mb = CreateMultifieldBuilder(self.env, arr.len());
                        if mb.is_null() {
                            return handle_err("Failed to create MultifieldBuilder");
                        }
                        for f in arr {
                            if (min.is_some() && *f < min.unwrap()) || (max.is_some() && *f > max.unwrap()) {
                                MBDispose(mb);
                                return handle_err("Value out of range");
                            }
                            MBAppendFloat(mb, *f);
                        }
                        let mf = MBCreate(mb);
                        MBDispose(mb);
                        match FMPutSlotMultifield(fm, CString::new("value")?.as_ptr(), mf) {
                            PutSlotError::None => {}
                            err => handle_err(&format!("PutSlot error: {:?}", err))?,
                        }
                    }
                    _ => return handle_err("Property type and value type mismatch"),
                },
                Property::StringArray { .. } => match value {
                    Value::Null => {
                        let mb = CreateMultifieldBuilder(self.env, 0);
                        if mb.is_null() {
                            return handle_err("Failed to create MultifieldBuilder");
                        }
                        let mf = MBCreate(mb);
                        MBDispose(mb);
                        match FMPutSlotMultifield(fm, CString::new("value")?.as_ptr(), mf) {
                            PutSlotError::None => {}
                            err => handle_err(&format!("PutSlot error: {:?}", err))?,
                        }
                    }
                    Value::StringArray(arr) => {
                        let mb = CreateMultifieldBuilder(self.env, arr.len());
                        if mb.is_null() {
                            return handle_err("Failed to create MultifieldBuilder");
                        }
                        for s in arr {
                            MBAppendString(mb, CString::new(s.clone())?.as_ptr());
                        }
                        let mf = MBCreate(mb);
                        MBDispose(mb);
                        match FMPutSlotMultifield(fm, CString::new("value")?.as_ptr(), mf) {
                            PutSlotError::None => {}
                            err => handle_err(&format!("PutSlot error: {:?}", err))?,
                        }
                    }
                    _ => return handle_err("Property type and value type mismatch"),
                },
                Property::SymbolArray { allowed_values, .. } => match value {
                    Value::Null => {
                        let mb = CreateMultifieldBuilder(self.env, 0);
                        if mb.is_null() {
                            return handle_err("Failed to create MultifieldBuilder");
                        }
                        let mf = MBCreate(mb);
                        MBDispose(mb);
                        match FMPutSlotMultifield(fm, CString::new("value")?.as_ptr(), mf) {
                            PutSlotError::None => {}
                            err => handle_err(&format!("PutSlot error: {:?}", err))?,
                        }
                    }
                    Value::StringArray(arr) => {
                        let mb = CreateMultifieldBuilder(self.env, arr.len());
                        if mb.is_null() {
                            return handle_err("Failed to create MultifieldBuilder");
                        }
                        for s in arr {
                            if let Some(allowed) = allowed_values
                                && !allowed.contains(s)
                            {
                                MBDispose(mb);
                                return handle_err("Value not in allowed values");
                            }
                            MBAppendSymbol(mb, CString::new(s.clone())?.as_ptr());
                        }
                        let mf = MBCreate(mb);
                        MBDispose(mb);
                        match FMPutSlotMultifield(fm, CString::new("value")?.as_ptr(), mf) {
                            PutSlotError::None => {}
                            err => handle_err(&format!("PutSlot error: {:?}", err))?,
                        }
                    }
                    _ => return handle_err("Property type and value type mismatch"),
                },
                Property::ObjectArray { class, .. } => match value {
                    Value::Null => {
                        let mb = CreateMultifieldBuilder(self.env, 0);
                        if mb.is_null() {
                            return handle_err("Failed to create MultifieldBuilder");
                        }
                        let mf = MBCreate(mb);
                        MBDispose(mb);
                        match FMPutSlotMultifield(fm, CString::new("value")?.as_ptr(), mf) {
                            PutSlotError::None => {}
                            err => handle_err(&format!("PutSlot error: {:?}", err))?,
                        }
                    }
                    Value::StringArray(arr) => {
                        let mb = CreateMultifieldBuilder(self.env, arr.len());
                        if mb.is_null() {
                            return handle_err("Failed to create MultifieldBuilder");
                        }
                        for o in arr {
                            if !self.instances.read().unwrap().get(class).is_some_and(|objs| objs.contains_key(o)) {
                                MBDispose(mb);
                                return handle_err("Object of specified class not found");
                            }
                            MBAppendSymbol(mb, CString::new(o.clone())?.as_ptr());
                        }
                        let mf = MBCreate(mb);
                        MBDispose(mb);
                        match FMPutSlotMultifield(fm, CString::new("value")?.as_ptr(), mf) {
                            PutSlotError::None => {}
                            err => handle_err(&format!("PutSlot error: {:?}", err))?,
                        }
                    }
                    _ => return handle_err("Property type and value type mismatch"),
                },
            }
            if let Some(t) = time {
                match FMPutSlotInteger(fm, CString::new("time".to_string())?.as_ptr(), t.timestamp()) {
                    PutSlotError::None => {}
                    err => handle_err(&format!("PutSlot error: {:?}", err))?,
                }
            }

            let modified_fact = FMModify(fm);
            if modified_fact.is_null() {
                return handle_err(&format!("Modification failed: {:?}", FMError(fm)));
            }
            self.facts.write().unwrap().get_mut(&class.name).and_then(|objs| objs.get_mut(object.id.as_ref().unwrap())).and_then(|props| props.get_mut(property_name)).map(|f| *f = modified_fact);

            FMDispose(fm);
            Ok(())
        }
    }
}

impl KnowledgeBase for CLIPSKnowledgeBase {
    fn get_event_sender(&self) -> broadcast::Sender<CoCoEvent> {
        self.sender.clone()
    }

    fn get_classes(&self) -> Vec<Class> {
        self.classes.read().unwrap().values().cloned().collect()
    }

    fn get_class(&self, name: &str) -> Option<Class> {
        self.classes.read().unwrap().get(name).cloned()
    }

    fn create_class(&self, class: &Class) -> Result<(), Box<dyn Error>> {
        unsafe {
            match Build(self.env, CString::new(format!("(deftemplate {} (slot id (type SYMBOL)))", class.name))?.as_ptr()) {
                BuildError::None => {}
                err => return Err(format!("Build error: {:?}", err).into()),
            }
            if let Some(static_props) = &class.static_properties {
                for (name, prop) in static_props {
                    match Build(self.env, CString::new(prop_deftemplate(class, name, prop, true))?.as_ptr()) {
                        BuildError::None => {}
                        err => return Err(format!("Build error: {:?}", err).into()),
                    }
                }
            }
            if let Some(dynamic_props) = &class.dynamic_properties {
                for (name, prop) in dynamic_props {
                    match Build(self.env, CString::new(prop_deftemplate(class, name, prop, false))?.as_ptr()) {
                        BuildError::None => {}
                        err => return Err(format!("Build error: {:?}", err).into()),
                    }
                }
            }
            self.classes.write().unwrap().insert(class.name.clone(), class.clone());
            let _ = self.sender.send(CoCoEvent::ClassCreated(class.clone()));
            Ok(())
        }
    }

    fn get_objects(&self) -> Vec<Object> {
        self.objects.read().unwrap().values().cloned().collect()
    }

    fn get_object(&self, id: &str) -> Option<Object> {
        self.objects.read().unwrap().get(id).cloned()
    }

    fn create_object(&self, object: &Object) -> Result<(), Box<dyn Error>> {
        let classes_guard = self.classes.read();
        for class_name in &object.classes {
            let class = classes_guard.as_ref().unwrap().get(class_name).ok_or("Class not found")?;
            unsafe {
                let fb = CreateFactBuilder(self.env, CString::new(class_name.to_string())?.as_ptr());
                if fb.is_null() {
                    return Err("Failed to create FactBuilder".into());
                }

                match FBPutSlotSymbol(fb, CString::new("id")?.as_ptr(), CString::new(object.id.as_ref().unwrap().clone())?.as_ptr()) {
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

                self.instances.write().unwrap().entry(class_name.to_string()).or_default().insert(object.id.as_ref().unwrap().clone(), fact);

                FBDispose(fb);

                if let Some(props) = class.static_properties.as_ref() {
                    for (prop_name, prop) in props {
                        let value = object.properties.as_ref().and_then(|props| props.get(prop_name));
                        if let Some(v) = value {
                            self.set_prop(object, &class, prop, prop_name, v, None)?;
                        } else {
                            let default_val = match prop {
                                Property::Bool { default: Some(v), .. } => Some(Value::Bool(*v)),
                                Property::Int { default: Some(v), .. } => Some(Value::Int(*v)),
                                Property::Float { default: Some(v), .. } => Some(Value::Float(*v)),
                                Property::String { default: Some(v), .. } => Some(Value::String(v.clone())),
                                Property::Symbol { default: Some(v), .. } => Some(Value::Symbol(v.clone())),
                                Property::Object { default: Some(v), .. } => Some(Value::Object(v.clone())),
                                Property::BoolArray { default: Some(v), .. } => Some(Value::BoolArray(v.clone())),
                                Property::IntArray { default: Some(v), .. } => Some(Value::IntArray(v.clone())),
                                Property::FloatArray { default: Some(v), .. } => Some(Value::FloatArray(v.clone())),
                                Property::StringArray { default: Some(v), .. } => Some(Value::StringArray(v.clone())),
                                Property::SymbolArray { default: Some(v), .. } => Some(Value::StringArray(v.clone())),
                                Property::ObjectArray { default: Some(v), .. } => Some(Value::StringArray(v.clone())),
                                _ => None,
                            };
                            if let Some(v) = default_val {
                                self.set_prop(object, &class, prop, prop_name, &v, None)?;
                            } else {
                                self.set_prop(object, &class, prop, prop_name, &Value::Null, None)?;
                            }
                        }
                    }
                }

                if let Some(props) = class.dynamic_properties.as_ref() {
                    for (prop_name, prop) in props {
                        let value_time = object.values.as_ref().and_then(|vals| vals.get(prop_name));
                        if let Some((v, t)) = value_time {
                            self.set_prop(object, &class, prop, prop_name, v, Some(t))?;
                        } else {
                            let default_val = match prop {
                                Property::Bool { default: Some(v), .. } => Some(Value::Bool(*v)),
                                Property::Int { default: Some(v), .. } => Some(Value::Int(*v)),
                                Property::Float { default: Some(v), .. } => Some(Value::Float(*v)),
                                Property::String { default: Some(v), .. } => Some(Value::String(v.clone())),
                                Property::Symbol { default: Some(v), .. } => Some(Value::Symbol(v.clone())),
                                Property::Object { default: Some(v), .. } => Some(Value::Object(v.clone())),
                                Property::BoolArray { default: Some(v), .. } => Some(Value::BoolArray(v.clone())),
                                Property::IntArray { default: Some(v), .. } => Some(Value::IntArray(v.clone())),
                                Property::FloatArray { default: Some(v), .. } => Some(Value::FloatArray(v.clone())),
                                Property::StringArray { default: Some(v), .. } => Some(Value::StringArray(v.clone())),
                                Property::SymbolArray { default: Some(v), .. } => Some(Value::StringArray(v.clone())),
                                Property::ObjectArray { default: Some(v), .. } => Some(Value::StringArray(v.clone())),
                                _ => None,
                            };
                            if let Some(v) = default_val {
                                self.set_prop(object, &class, prop, prop_name, &v, None)?;
                            } else {
                                self.set_prop(object, &class, prop, prop_name, &Value::Null, None)?;
                            }
                        }
                    }
                }
            }
        }
        self.objects.write().unwrap().insert(object.id.as_ref().unwrap().clone(), object.clone());
        let _ = self.sender.send(CoCoEvent::ObjectCreated(object.clone()));
        Ok(())
    }

    fn set_properties(&self, object: &mut Object, values: HashMap<String, Value>) -> Result<(), Box<dyn Error>> {
        let classes_guard = self.classes.read();
        for class_name in &object.classes {
            let class = classes_guard.as_ref().unwrap().get(class_name).ok_or("Class not found")?;
            if let Some(props) = class.static_properties.as_ref() {
                for (prop_name, value) in values.clone() {
                    if let Some(prop) = props.get(&prop_name) {
                        self.update_prop(object, class, &prop, &prop_name, &value, None)?;
                        object.properties.as_mut().unwrap().insert(prop_name, value);
                    }
                }
            }
        }
        let _ = self.sender.send(CoCoEvent::UpdatedProperties(object.id.as_ref().unwrap().clone(), values));
        Ok(())
    }

    fn add_data(&self, object: &mut Object, values: HashMap<String, Value>, date_time: DateTime<Utc>) -> Result<(), Box<dyn Error>> {
        let classes_guard = self.classes.read();
        for class_name in &object.classes {
            let class = classes_guard.as_ref().unwrap().get(class_name).ok_or("Class not found")?;
            if let Some(props) = class.dynamic_properties.as_ref() {
                for (prop_name, value) in values.clone() {
                    if let Some(prop) = props.get(&prop_name) {
                        self.update_prop(object, class, &prop, &prop_name, &value, Some(&date_time))?;
                        object.values.as_mut().unwrap().insert(prop_name, (value, date_time));
                    }
                }
            }
        }
        let _ = self.sender.send(CoCoEvent::AddedValues(object.id.as_ref().unwrap().clone(), values.iter().map(|(k, v)| (k.clone(), (v.clone(), date_time))).collect()));
        Ok(())
    }

    fn get_rules(&self) -> Vec<Rule> {
        self.rules.read().unwrap().values().cloned().collect()
    }

    fn get_rule(&self, name: &str) -> Option<Rule> {
        self.rules.read().unwrap().get(name).cloned()
    }

    fn create_rule(&self, rule: &Rule) -> Result<(), Box<dyn Error>> {
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
                let def_str = def_val.to_string();
                let def_str = if def_str.contains('.') { def_str } else { format!("{}.0", def_str) };
                def.push_str(&format!(" (default {})", def_str));
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
        Property::BoolArray { default } => {
            def.push_str(" (multislot value (type SYMBOL) (allowed-symbols TRUE FALSE)");
            if let Some(def_val) = default {
                let def_str = def_val.iter().map(|b| if *b { "TRUE" } else { "FALSE" }).collect::<Vec<_>>().join(" ");
                def.push_str(&format!(" (default {})", def_str));
            }
            def.push(')');
            if !is_static {
                def.push_str(" (slot time (type INTEGER))");
            }
            def.push(')');
            def
        }
        Property::IntArray { default, min, max } => {
            def.push_str(" (multislot value (type INTEGER)");
            if let Some(def_val) = default {
                let def_str = def_val.iter().map(|i| i.to_string()).collect::<Vec<_>>().join(" ");
                def.push_str(&format!(" (default {})", def_str));
            }
            if min.is_some() || max.is_some() {
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
        Property::FloatArray { default, min, max } => {
            def.push_str(" (multislot value (type FLOAT)");
            if let Some(def_val) = default {
                let def_str = def_val
                    .iter()
                    .map(|f| {
                        let s = f.to_string();
                        if s.contains('.') { s } else { format!("{}.0", s) }
                    })
                    .collect::<Vec<_>>()
                    .join(" ");
                def.push_str(&format!(" (default {})", def_str));
            }
            if min.is_some() || max.is_some() {
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
        Property::StringArray { default } => {
            def.push_str(" (multislot value (type STRING)");
            if let Some(def_val) = default {
                let def_str = def_val.iter().map(|s| format!("\"{}\"", s)).collect::<Vec<_>>().join(" ");
                def.push_str(&format!(" (default {})", def_str));
            }
            def.push(')');
            if !is_static {
                def.push_str(" (slot time (type INTEGER))");
            }
            def.push(')');
            def
        }
        Property::SymbolArray { default, allowed_values } => {
            def.push_str(" (multislot value (type SYMBOL)");
            if let Some(allowed) = allowed_values {
                def.push_str(" (allowed-symbols");
                for v in allowed {
                    def.push_str(&format!(" {}", v));
                }
                def.push(')');
            }
            if let Some(def_val) = default {
                let def_str = def_val.iter().map(|s| s.as_str()).collect::<Vec<_>>().join(" ");
                def.push_str(&format!(" (default {})", def_str));
            }
            def.push(')');
            if !is_static {
                def.push_str(" (slot time (type INTEGER))");
            }
            def.push(')');
            def
        }
        Property::ObjectArray { default, .. } => {
            def.push_str(" (multislot value (type SYMBOL)");
            if let Some(def_val) = default {
                let def_str = def_val.iter().map(|o| o.as_str()).collect::<Vec<_>>().join(" ");
                def.push_str(&format!(" (default {})", def_str));
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

        let time = if !(*udfc).last_arg.is_null() {
            let mut time_val = std::mem::MaybeUninit::<UDFValue>::uninit();
            if !UDFNextArgument(udfc, CLIPSType::IntegerBit, time_val.as_mut_ptr()) {
                return;
            }
            let time_val = time_val.assume_init();
            DateTime::<Utc>::from_timestamp((*time_val.value.integer_value).contents, 0).unwrap()
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
                        s => {
                            values.insert(par_name.to_string(), (Value::Symbol(s.to_string()), time));
                        }
                    }
                }
                CLIPSTypeCode::Integer => {
                    values.insert(par_name.to_string(), (Value::Int((*val.value.integer_value).contents), time));
                }
                CLIPSTypeCode::Float => {
                    values.insert(par_name.to_string(), (Value::Float((*val.value.float_value).contents), time));
                }
                CLIPSTypeCode::String => {
                    let val_str = std::ffi::CStr::from_ptr((*val.value.lexeme_value).contents).to_str().unwrap();
                    values.insert(par_name.to_string(), (Value::String(val_str.to_string()), time));
                }
                _ => {}
            }
        }
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

#[cfg(test)]
mod tests {
    use super::*;
    use std::collections::{HashMap, HashSet};
    use tokio::sync::broadcast;

    fn create_kb() -> CLIPSKnowledgeBase {
        let (tx, _) = broadcast::channel(100);
        let kb = CLIPSKnowledgeBase::new(tx);
        kb.init();
        kb
    }

    #[test]
    fn test_kb_initialization() {
        let kb = create_kb();
        assert!(!kb.env.is_null());
    }

    #[test]
    fn test_create_class() {
        let kb = create_kb();
        let mut static_props = HashMap::new();
        static_props.insert("name".to_string(), Property::String { nullable: Some(false), default: Some("default property".to_string()) });
        let class = Class {
            name: "TestClass".to_string(),
            parents: None,
            static_properties: Some(static_props),
            dynamic_properties: None,
        };
        assert!(kb.create_class(&class).is_ok());
    }

    #[test]
    fn test_create_object() {
        let kb = create_kb();

        let mut static_props = HashMap::new();
        static_props.insert("s_prop".to_string(), Property::String { nullable: Some(true), default: None });
        let class = Class {
            name: "Person".to_string(),
            parents: None,
            static_properties: Some(static_props),
            dynamic_properties: None,
        };
        kb.create_class(&class).unwrap();

        let mut classes = HashSet::new();
        classes.insert("Person".to_string());

        let mut props = HashMap::new();
        props.insert("s_prop".to_string(), Value::String("value1".to_string()));

        let object = Object { id: Some("person1".to_string()), classes, properties: Some(props), values: None };

        assert!(kb.create_object(&object).is_ok());
    }

    #[test]
    fn test_set_properties() {
        let kb = create_kb();

        let mut static_props = HashMap::new();
        static_props.insert("age".to_string(), Property::Int { nullable: Some(false), default: Some(0), min: Some(0), max: Some(150) });
        let class = Class {
            name: "Person".to_string(),
            parents: None,
            static_properties: Some(static_props),
            dynamic_properties: None,
        };
        kb.create_class(&class).unwrap();

        let mut classes = HashSet::new();
        classes.insert("Person".to_string());

        // Initial creation with default
        let mut object = Object {
            id: Some("person1".to_string()),
            classes,
            properties: Some(HashMap::new()), // Empty map means use defaults/nulls as per logic
            values: None,
        };
        kb.create_object(&object).unwrap();

        // Update properties
        let mut new_props = HashMap::new();
        new_props.insert("age".to_string(), Value::Int(30));
        assert!(kb.set_properties(&mut object, new_props).is_ok());
    }

    #[test]
    fn test_add_data() {
        let kb = create_kb();

        let mut dynamic_props = HashMap::new();
        dynamic_props.insert("temperature".to_string(), Property::Float { nullable: Some(false), default: Some(0.0), min: Some(-100.0), max: Some(100.0) });
        let class = Class {
            name: "Sensor".to_string(),
            parents: None,
            static_properties: None,
            dynamic_properties: Some(dynamic_props),
        };
        kb.create_class(&class).unwrap();

        let mut classes = HashSet::new();
        classes.insert("Sensor".to_string());
        let mut object = Object { id: Some("sensor1".to_string()), classes, properties: None, values: Some(HashMap::new()) };
        kb.create_object(&object).unwrap();

        // Add data
        let mut values = HashMap::new();
        values.insert("temperature".to_string(), Value::Float(25.5));
        assert!(kb.add_data(&mut object, values, Utc::now()).is_ok());
    }

    #[test]
    fn test_various_property_types() {
        let kb = create_kb();

        let mut static_props = HashMap::new();
        static_props.insert("p_int".to_string(), Property::Int { nullable: Some(false), default: None, min: None, max: None });
        static_props.insert("p_float".to_string(), Property::Float { nullable: Some(false), default: None, min: None, max: None });
        static_props.insert("p_bool".to_string(), Property::Bool { nullable: Some(false), default: None });
        static_props.insert("p_string".to_string(), Property::String { nullable: Some(false), default: None });
        static_props.insert("p_symbol".to_string(), Property::Symbol { nullable: Some(false), default: None, allowed_values: None });

        let class = Class {
            name: "AllTypes".to_string(),
            parents: None,
            static_properties: Some(static_props),
            dynamic_properties: None,
        };
        kb.create_class(&class).unwrap();

        let mut classes = HashSet::new();
        classes.insert("AllTypes".to_string());

        let mut props = HashMap::new();
        props.insert("p_int".to_string(), Value::Int(42));
        props.insert("p_float".to_string(), Value::Float(3.14));
        props.insert("p_bool".to_string(), Value::Bool(true));
        props.insert("p_string".to_string(), Value::String("hello".to_string()));
        props.insert("p_symbol".to_string(), Value::Symbol("sym".to_string()));

        let object = Object { id: Some("obj1".to_string()), classes, properties: Some(props), values: None };

        assert!(kb.create_object(&object).is_ok());
    }

    #[test]
    fn test_range_validation() {
        let kb = create_kb();
        let mut static_props = HashMap::new();
        static_props.insert("p_int".to_string(), Property::Int { nullable: Some(false), default: None, min: Some(10), max: Some(20) });
        let class = Class {
            name: "RangeTest".to_string(),
            parents: None,
            static_properties: Some(static_props),
            dynamic_properties: None,
        };
        kb.create_class(&class).unwrap();
        let mut classes = HashSet::new();
        classes.insert("RangeTest".to_string());

        // Valid
        let mut props_valid = HashMap::new();
        props_valid.insert("p_int".to_string(), Value::Int(15));
        let obj_valid = Object {
            id: Some("ok".to_string()),
            classes: classes.clone(),
            properties: Some(props_valid),
            values: None,
        };
        assert!(kb.create_object(&obj_valid).is_ok());

        // Invalid (below min)
        let mut props_invalid = HashMap::new();
        props_invalid.insert("p_int".to_string(), Value::Int(5));
        let obj_invalid = Object {
            id: Some("fail".to_string()),
            classes: classes.clone(),
            properties: Some(props_invalid),
            values: None,
        };
        assert!(kb.create_object(&obj_invalid).is_err());
    }

    #[test]
    fn test_create_rule() {
        let kb = create_kb();
        let rule = Rule {
            name: "test-rule".to_string(),
            content: "(defrule test-rule => (printout t \"Hello\" crlf))".to_string(),
        };
        assert!(kb.create_rule(&rule).is_ok());
    }

    #[test]
    fn test_array_property_types() {
        let kb = create_kb();

        // Setup for ObjectArray
        let item_class = Class { name: "Item".to_string(), parents: None, static_properties: None, dynamic_properties: None };
        kb.create_class(&item_class).unwrap();

        let mut item_classes = HashSet::new();
        item_classes.insert("Item".to_string());

        let item1 = Object {
            id: Some("item1".to_string()),
            classes: item_classes.clone(),
            properties: None,
            values: None,
        };
        let item2 = Object {
            id: Some("item2".to_string()),
            classes: item_classes.clone(),
            properties: None,
            values: None,
        };
        kb.create_object(&item1).unwrap();
        kb.create_object(&item2).unwrap();

        let mut static_props = HashMap::new();
        static_props.insert("p_bool_arr".to_string(), Property::BoolArray { default: None });
        static_props.insert("p_int_arr".to_string(), Property::IntArray { default: None, min: None, max: None });
        static_props.insert("p_float_arr".to_string(), Property::FloatArray { default: None, min: None, max: None });
        static_props.insert("p_string_arr".to_string(), Property::StringArray { default: None });
        static_props.insert("p_symbol_arr".to_string(), Property::SymbolArray { default: None, allowed_values: None });
        static_props.insert("p_obj_arr".to_string(), Property::ObjectArray { default: None, class: "Item".to_string() });

        let class = Class {
            name: "ArrayTypes".to_string(),
            parents: None,
            static_properties: Some(static_props),
            dynamic_properties: None,
        };
        kb.create_class(&class).unwrap();

        let mut classes = HashSet::new();
        classes.insert("ArrayTypes".to_string());

        let mut props = HashMap::new();
        props.insert("p_bool_arr".to_string(), Value::BoolArray(vec![true, false, true]));
        props.insert("p_int_arr".to_string(), Value::IntArray(vec![1, 2, 3]));
        props.insert("p_float_arr".to_string(), Value::FloatArray(vec![1.1, 2.2, 3.3]));
        props.insert("p_string_arr".to_string(), Value::StringArray(vec!["s1".to_string(), "s2".to_string()]));
        props.insert("p_symbol_arr".to_string(), Value::StringArray(vec!["sym1".to_string(), "sym2".to_string()]));
        props.insert("p_obj_arr".to_string(), Value::StringArray(vec!["item1".to_string(), "item2".to_string()]));

        let object = Object { id: Some("arr_obj".to_string()), classes, properties: Some(props), values: None };

        assert!(kb.create_object(&object).is_ok());
    }
}

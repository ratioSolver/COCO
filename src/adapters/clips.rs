use crate::{Class, CoCoEvent, KnowledgeBase, Value};
use std::{
    collections::HashMap,
    ffi::{CString, c_char, c_double, c_long, c_longlong, c_ushort},
    marker::{PhantomData, PhantomPinned},
    os::raw::c_void,
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

pub struct CLIPSKnowledgeBase {
    sender: broadcast::Sender<CoCoEvent>,
    env: *mut Environment,
    instances: HashMap<String, HashMap<String, *mut Fact>>,              // class -> object -> fact
    facts: HashMap<String, HashMap<String, HashMap<String, *mut Fact>>>, // class -> object -> property -> fact
}

unsafe impl Send for CLIPSKnowledgeBase {}
unsafe impl Sync for CLIPSKnowledgeBase {}

impl CLIPSKnowledgeBase {
    pub fn new(sender: broadcast::Sender<CoCoEvent>) -> Self {
        unsafe {
            let kb = CLIPSKnowledgeBase { sender, env: CreateEnvironment(), instances: HashMap::new(), facts: HashMap::new() };
            let boxed_sender = Box::new(kb.sender.clone());
            let context_ptr = Box::into_raw(boxed_sender) as *mut std::ffi::c_void;
            AddUDF(kb.env, CString::new("add-values").unwrap().as_ptr(), CString::new("v").unwrap().as_ptr(), 0, 0, CString::new("").unwrap().as_ptr(), add_values, CString::new("add-values").unwrap().as_ptr(), context_ptr);
            AddUDF(kb.env, CString::new("add-class").unwrap().as_ptr(), CString::new("v").unwrap().as_ptr(), 0, 0, CString::new("").unwrap().as_ptr(), add_class, CString::new("add-class").unwrap().as_ptr(), context_ptr);
            kb
        }
    }
}

impl KnowledgeBase for CLIPSKnowledgeBase {
    fn get_event_sender(&self) -> broadcast::Sender<CoCoEvent> {
        self.sender.clone()
    }

    fn create_class(&self, class: &Class) {}
}

unsafe extern "C" fn add_values(_env: *mut Environment, _udfc: *mut UDFContext, _out: *mut UDFValue) {
    unsafe {
        let sender = &*((*_udfc).context as *mut broadcast::Sender<CoCoEvent>);
        let _ = sender.send(CoCoEvent::AddedValues("Object1".to_string(), [("property1".to_string(), (Value::Bool(true), chrono::Utc::now()))].iter().cloned().collect()));
    }
}

unsafe extern "C" fn add_class(_env: *mut Environment, _udfc: *mut UDFContext, _out: *mut UDFValue) {
    unsafe {
        let sender = &*((*_udfc).context as *mut broadcast::Sender<CoCoEvent>);
        let _ = sender.send(CoCoEvent::ClassCreated(Class {
            name: "TestClass".to_string(),
            parents: None,
            static_properties: None,
            dynamic_properties: None,
        }));
    }
}

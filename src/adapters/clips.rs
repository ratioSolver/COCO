#![allow(non_upper_case_globals)]
#![allow(non_camel_case_types)]
#![allow(non_snake_case)]

#[allow(non_upper_case_globals)]
#[allow(non_camel_case_types)]
#[allow(non_snake_case)]
#[allow(dead_code)]
#[allow(unsafe_op_in_unsafe_fn)]
mod bindings {
    include!("clips_bindings.rs");
}
pub use bindings::*;

use crate::{Class, CoCoEvent, KnowledgeBase, Object, Property, Rule, Value};
use chrono::{DateTime, Utc};
use std::{collections::HashMap, error::Error, ffi::CString, os::raw::c_void, sync::RwLock};
use tokio::sync::broadcast;

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
            AddUDF(self.env, CString::new("add-data").unwrap().as_ptr(), CString::new("v").unwrap().as_ptr(), 3, 4, CString::new("ymml").unwrap().as_ptr(), Some(add_data), CString::new("add_data").unwrap().as_ptr(), self as *const _ as *mut c_void);
            AddUDF(self.env, CString::new("add-class").unwrap().as_ptr(), CString::new("v").unwrap().as_ptr(), 2, 2, CString::new("yy").unwrap().as_ptr(), Some(add_class), CString::new("add_class").unwrap().as_ptr(), self as *const _ as *mut c_void);
        }
    }

    pub fn add_udf(&self, name: &str, return_types: &str, min_args: u16, max_args: u16, arg_types: &str, function_ptr: UserDefinedFunction, r_name: &str) -> Result<(), Box<dyn Error>> {
        unsafe {
            let result = AddUDF(self.env, CString::new(name)?.as_ptr(), CString::new(return_types)?.as_ptr(), min_args, max_args, CString::new(arg_types)?.as_ptr(), function_ptr, CString::new(r_name)?.as_ptr(), self as *const _ as *mut c_void);
            match result {
                AddUDFError_AUE_NO_ERROR => Ok(()),
                _ => Err(format!("AddUDF error: {:?}", result).into()),
            }
        }
    }

    fn create_object_class(&self, object: &Object, class: &Class) -> Result<(), Box<dyn Error>> {
        unsafe {
            let fb = CreateFactBuilder(self.env, CString::new(class.name.clone())?.as_ptr());
            if fb.is_null() {
                return Err("Failed to create FactBuilder".into());
            }

            match FBPutSlotSymbol(fb, CString::new("id")?.as_ptr(), CString::new(object.id.as_ref().unwrap().clone())?.as_ptr()) {
                PutSlotError_PSE_NO_ERROR => {}
                err => {
                    FBDispose(fb);
                    return Err(format!("PutSlot error: {:?}", err).into());
                }
            }

            let fact = FBAssert(fb);
            if fact.is_null() {
                let error = FBError(self.env);
                FBDispose(fb);
                return Err(format!("Assertion failed: {:?}", error).into());
            }

            self.instances.write().unwrap().entry(class.name.clone()).or_default().insert(object.id.as_ref().unwrap().clone(), fact);

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

        Ok(())
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
                PutSlotError_PSE_NO_ERROR => {}
                err => handle_err(&format!("PutSlot error: {:?}", err))?,
            }
            match property {
                Property::Bool { nullable, .. } => match value {
                    Value::Null => {
                        if let Some(true) = nullable {
                            match FBPutSlotSymbol(fb, CString::new("value")?.as_ptr(), CString::new("nil")?.as_ptr()) {
                                PutSlotError_PSE_NO_ERROR => {}
                                err => handle_err(&format!("PutSlot error: {:?}", err))?,
                            }
                        } else {
                            return handle_err("Cannot assign null to non-nullable property");
                        }
                    }
                    Value::Bool(b) => {
                        let symbol = if *b { "TRUE" } else { "FALSE" };
                        match FBPutSlotSymbol(fb, CString::new("value")?.as_ptr(), CString::new(symbol)?.as_ptr()) {
                            PutSlotError_PSE_NO_ERROR => {}
                            err => handle_err(&format!("PutSlot error: {:?}", err))?,
                        }
                    }
                    _ => return handle_err("Property type and value type mismatch"),
                },
                Property::Int { nullable, min, max, .. } => match value {
                    Value::Null => {
                        if let Some(true) = nullable {
                            match FBPutSlotSymbol(fb, CString::new("value")?.as_ptr(), CString::new("nil")?.as_ptr()) {
                                PutSlotError_PSE_NO_ERROR => {}
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
                            PutSlotError_PSE_NO_ERROR => {}
                            err => handle_err(&format!("PutSlot error: {:?}", err))?,
                        }
                    }
                    _ => return handle_err("Property type and value type mismatch"),
                },
                Property::Float { nullable, min, max, .. } => match value {
                    Value::Null => {
                        if let Some(true) = nullable {
                            match FBPutSlotSymbol(fb, CString::new("value")?.as_ptr(), CString::new("nil")?.as_ptr()) {
                                PutSlotError_PSE_NO_ERROR => {}
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
                            PutSlotError_PSE_NO_ERROR => {}
                            err => handle_err(&format!("PutSlot error: {:?}", err))?,
                        }
                    }
                    _ => return handle_err("Property type and value type mismatch"),
                },
                Property::String { nullable, .. } => match value {
                    Value::Null => {
                        if let Some(true) = nullable {
                            match FBPutSlotSymbol(fb, CString::new("value")?.as_ptr(), CString::new("nil")?.as_ptr()) {
                                PutSlotError_PSE_NO_ERROR => {}
                                err => handle_err(&format!("PutSlot error: {:?}", err))?,
                            }
                        } else {
                            return handle_err("Cannot assign null to non-nullable property");
                        }
                    }
                    Value::String(s) => match FBPutSlotString(fb, CString::new("value")?.as_ptr(), CString::new(s.clone())?.as_ptr()) {
                        PutSlotError_PSE_NO_ERROR => {}
                        err => handle_err(&format!("PutSlot error: {:?}", err))?,
                    },
                    _ => return handle_err("Property type and value type mismatch"),
                },
                Property::Symbol { nullable, allowed_values, .. } => match value {
                    Value::Null => {
                        if let Some(true) = nullable {
                            match FBPutSlotSymbol(fb, CString::new("value")?.as_ptr(), CString::new("nil")?.as_ptr()) {
                                PutSlotError_PSE_NO_ERROR => {}
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
                            PutSlotError_PSE_NO_ERROR => {}
                            err => handle_err(&format!("PutSlot error: {:?}", err))?,
                        }
                    }
                    _ => return handle_err("Property type and value type mismatch"),
                },
                Property::Object { nullable, class, .. } => match value {
                    Value::Null => {
                        if let Some(true) = nullable {
                            match FBPutSlotSymbol(fb, CString::new("value")?.as_ptr(), CString::new("nil")?.as_ptr()) {
                                PutSlotError_PSE_NO_ERROR => {}
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
                            PutSlotError_PSE_NO_ERROR => {}
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
                                PutSlotError_PSE_NO_ERROR => {}
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
                            PutSlotError_PSE_NO_ERROR => {}
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
                                PutSlotError_PSE_NO_ERROR => {}
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
                            PutSlotError_PSE_NO_ERROR => {}
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
                                PutSlotError_PSE_NO_ERROR => {}
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
                            PutSlotError_PSE_NO_ERROR => {}
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
                                PutSlotError_PSE_NO_ERROR => {}
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
                            PutSlotError_PSE_NO_ERROR => {}
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
                                PutSlotError_PSE_NO_ERROR => {}
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
                            PutSlotError_PSE_NO_ERROR => {}
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
                                PutSlotError_PSE_NO_ERROR => {}
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
                            PutSlotError_PSE_NO_ERROR => {}
                            err => handle_err(&format!("PutSlot error: {:?}", err))?,
                        }
                    }
                    _ => return handle_err("Property type and value type mismatch"),
                },
            }
            if let Some(t) = time {
                match FBPutSlotInteger(fb, CString::new("time".to_string())?.as_ptr(), t.timestamp()) {
                    PutSlotError_PSE_NO_ERROR => {}
                    err => handle_err(&format!("PutSlot error: {:?}", err))?,
                }
            }

            let fact = FBAssert(fb);
            if fact.is_null() {
                return handle_err(&format!("Assertion failed: {:?}", FBError(self.env)));
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
                PutSlotError_PSE_NO_ERROR => {}
                err => handle_err(&format!("PutSlot error: {:?}", err))?,
            }
            match property {
                Property::Bool { nullable, .. } => match value {
                    Value::Null => {
                        if let Some(true) = nullable {
                            match FMPutSlotSymbol(fm, CString::new("value")?.as_ptr(), CString::new("nil")?.as_ptr()) {
                                PutSlotError_PSE_NO_ERROR => {}
                                err => handle_err(&format!("PutSlot error: {:?}", err))?,
                            }
                        } else {
                            return handle_err("Cannot assign null to non-nullable property");
                        }
                    }
                    Value::Bool(b) => {
                        let symbol = if *b { "TRUE" } else { "FALSE" };
                        match FMPutSlotSymbol(fm, CString::new("value")?.as_ptr(), CString::new(symbol)?.as_ptr()) {
                            PutSlotError_PSE_NO_ERROR => {}
                            err => handle_err(&format!("PutSlot error: {:?}", err))?,
                        }
                    }
                    _ => return handle_err("Property type and value type mismatch"),
                },
                Property::Int { nullable, min, max, .. } => match value {
                    Value::Null => {
                        if let Some(true) = nullable {
                            match FMPutSlotSymbol(fm, CString::new("value")?.as_ptr(), CString::new("nil")?.as_ptr()) {
                                PutSlotError_PSE_NO_ERROR => {}
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
                            PutSlotError_PSE_NO_ERROR => {}
                            err => handle_err(&format!("PutSlot error: {:?}", err))?,
                        }
                    }
                    _ => return handle_err("Property type and value type mismatch"),
                },
                Property::Float { nullable, min, max, .. } => match value {
                    Value::Null => {
                        if let Some(true) = nullable {
                            match FMPutSlotSymbol(fm, CString::new("value")?.as_ptr(), CString::new("nil")?.as_ptr()) {
                                PutSlotError_PSE_NO_ERROR => {}
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
                            PutSlotError_PSE_NO_ERROR => {}
                            err => handle_err(&format!("PutSlot error: {:?}", err))?,
                        }
                    }
                    _ => return handle_err("Property type and value type mismatch"),
                },
                Property::String { nullable, .. } => match value {
                    Value::Null => {
                        if let Some(true) = nullable {
                            match FMPutSlotSymbol(fm, CString::new("value")?.as_ptr(), CString::new("nil")?.as_ptr()) {
                                PutSlotError_PSE_NO_ERROR => {}
                                err => handle_err(&format!("PutSlot error: {:?}", err))?,
                            }
                        } else {
                            return handle_err("Cannot assign null to non-nullable property");
                        }
                    }
                    Value::String(s) => match FMPutSlotString(fm, CString::new("value")?.as_ptr(), CString::new(s.clone())?.as_ptr()) {
                        PutSlotError_PSE_NO_ERROR => {}
                        err => handle_err(&format!("PutSlot error: {:?}", err))?,
                    },
                    _ => return handle_err("Property type and value type mismatch"),
                },
                Property::Symbol { nullable, allowed_values, .. } => match value {
                    Value::Null => {
                        if let Some(true) = nullable {
                            match FMPutSlotSymbol(fm, CString::new("value")?.as_ptr(), CString::new("nil")?.as_ptr()) {
                                PutSlotError_PSE_NO_ERROR => {}
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
                            PutSlotError_PSE_NO_ERROR => {}
                            err => handle_err(&format!("PutSlot error: {:?}", err))?,
                        }
                    }
                    _ => return handle_err("Property type and value type mismatch"),
                },
                Property::Object { nullable, class, .. } => match value {
                    Value::Null => {
                        if let Some(true) = nullable {
                            match FMPutSlotSymbol(fm, CString::new("value")?.as_ptr(), CString::new("nil")?.as_ptr()) {
                                PutSlotError_PSE_NO_ERROR => {}
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
                            PutSlotError_PSE_NO_ERROR => {}
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
                            PutSlotError_PSE_NO_ERROR => {}
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
                            PutSlotError_PSE_NO_ERROR => {}
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
                            PutSlotError_PSE_NO_ERROR => {}
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
                            PutSlotError_PSE_NO_ERROR => {}
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
                            PutSlotError_PSE_NO_ERROR => {}
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
                            PutSlotError_PSE_NO_ERROR => {}
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
                            PutSlotError_PSE_NO_ERROR => {}
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
                            PutSlotError_PSE_NO_ERROR => {}
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
                            PutSlotError_PSE_NO_ERROR => {}
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
                            PutSlotError_PSE_NO_ERROR => {}
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
                            PutSlotError_PSE_NO_ERROR => {}
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
                            PutSlotError_PSE_NO_ERROR => {}
                            err => handle_err(&format!("PutSlot error: {:?}", err))?,
                        }
                    }
                    _ => return handle_err("Property type and value type mismatch"),
                },
            }
            if let Some(t) = time {
                match FMPutSlotInteger(fm, CString::new("time".to_string())?.as_ptr(), t.timestamp()) {
                    PutSlotError_PSE_NO_ERROR => {}
                    err => handle_err(&format!("PutSlot error: {:?}", err))?,
                }
            }

            let modified_fact = FMModify(fm);
            if modified_fact.is_null() {
                return handle_err(&format!("Modification failed: {:?}", FMError(self.env)));
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
                BuildError_BE_NO_ERROR => {}
                err => return Err(format!("Build error: {:?}", err).into()),
            }
            if let Some(static_props) = &class.static_properties {
                for (name, prop) in static_props {
                    match Build(self.env, CString::new(prop_deftemplate(class, name, prop, true))?.as_ptr()) {
                        BuildError_BE_NO_ERROR => {}
                        err => return Err(format!("Build error: {:?}", err).into()),
                    }
                }
            }
            if let Some(dynamic_props) = &class.dynamic_properties {
                for (name, prop) in dynamic_props {
                    match Build(self.env, CString::new(prop_deftemplate(class, name, prop, false))?.as_ptr()) {
                        BuildError_BE_NO_ERROR => {}
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
            self.create_object_class(object, classes_guard.as_ref().unwrap().get(class_name).ok_or("Class not found")?)?;
        }

        self.objects.write().unwrap().insert(object.id.as_ref().unwrap().clone(), object.clone());
        let _ = self.sender.send(CoCoEvent::ObjectCreated(object.clone()));
        Ok(())
    }

    fn add_class(&self, object_id: &str, class_name: &str) -> Result<(), Box<dyn Error>> {
        let classes_guard = self.classes.read().unwrap();
        if !classes_guard.contains_key(class_name) {
            return Err("Class not found".into());
        }

        let mut objects_guard = self.objects.write().unwrap();
        let object = objects_guard.get_mut(object_id).ok_or("Object not found")?;
        if object.classes.contains(class_name) {
            return Ok(()); // Class already added, do nothing
        }
        let class = classes_guard.get(class_name).ok_or("Class not found")?;
        self.create_object_class(object, class)?;
        object.classes.insert(class_name.to_string());
        let _ = self.sender.send(CoCoEvent::AddedClass(object.clone(), class.clone()));
        Ok(())
    }

    fn set_properties(&self, object_id: &str, values: HashMap<String, Value>) -> Result<(), Box<dyn Error>> {
        let classes_guard = self.classes.read();
        let mut objects_guard = self.objects.write().unwrap();
        let object = objects_guard.get_mut(object_id).ok_or("Object not found")?;
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
        let _ = self.sender.send(CoCoEvent::UpdatedProperties(object.clone(), values));
        Ok(())
    }

    fn add_data(&self, object_id: &str, values: HashMap<String, Value>, date_time: DateTime<Utc>) -> Result<(), Box<dyn Error>> {
        let classes_guard = self.classes.read();
        let mut objects_guard = self.objects.write().unwrap();
        let object = objects_guard.get_mut(object_id).ok_or("Object not found")?;
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
        let _ = self.sender.send(CoCoEvent::AddedValues(object.clone(), values, date_time));
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
                BuildError_BE_NO_ERROR => Ok(()),
                err => Err(format!("Build error: {:?}", err).into()),
            }
        }
    }

    fn run(&self) -> Result<(), Box<dyn Error>> {
        unsafe {
            let _ = Run(self.env, -1);
            Ok(())
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

unsafe extern "C" fn add_data(_env: *mut Environment, udfc: *mut UDFContext, _out: *mut UDFValue) {
    unsafe {
        let kb = &*((*udfc).context as *mut CLIPSKnowledgeBase);
        let mut object_id = std::mem::MaybeUninit::<UDFValue>::uninit();
        if !UDFFirstArgument(udfc, CLIPSType_SYMBOL_BIT, object_id.as_mut_ptr()) {
            return;
        }
        let object_id = object_id.assume_init();
        if object_id.__bindgen_anon_1.header.is_null() {
            panic!("Received NULL header in UDF argument object_id in add_values");
        }
        let object_id = std::ffi::CStr::from_ptr((*object_id.__bindgen_anon_1.lexemeValue).contents).to_str().unwrap();

        let mut pars = std::mem::MaybeUninit::<UDFValue>::uninit();
        if !UDFNextArgument(udfc, CLIPSType_MULTIFIELD_BIT, pars.as_mut_ptr()) {
            return;
        }
        let pars = pars.assume_init();
        let mut vars = std::mem::MaybeUninit::<UDFValue>::uninit();
        if !UDFNextArgument(udfc, CLIPSType_MULTIFIELD_BIT, vars.as_mut_ptr()) {
            return;
        }
        let vars = vars.assume_init();

        let time = if !(*udfc).lastArg.is_null() {
            let mut time_val = std::mem::MaybeUninit::<UDFValue>::uninit();
            if !UDFNextArgument(udfc, CLIPSType_INTEGER_BIT, time_val.as_mut_ptr()) {
                return;
            }
            let time_val = time_val.assume_init();
            DateTime::<Utc>::from_timestamp((*time_val.__bindgen_anon_1.integerValue).contents, 0).unwrap()
        } else {
            Utc::now()
        };

        assert!((*pars.__bindgen_anon_1.multifieldValue).length == (*vars.__bindgen_anon_1.multifieldValue).length);
        let length = (*pars.__bindgen_anon_1.multifieldValue).length;
        let pars_contents = (*pars.__bindgen_anon_1.multifieldValue).contents;
        let vars_contents = (*vars.__bindgen_anon_1.multifieldValue).contents;

        let mut values = HashMap::new();
        for i in 0..length {
            let par = pars_contents.get(i).unwrap();
            let par_name = std::ffi::CStr::from_ptr((*par.__bindgen_anon_1.lexemeValue).contents).to_str().unwrap();
            let val = vars_contents.get(i).unwrap();
            match val.__bindgen_anon_1.header.as_ref().unwrap().type_ as u32 {
                SYMBOL_TYPE => {
                    let val_str = std::ffi::CStr::from_ptr((*val.__bindgen_anon_1.lexemeValue).contents).to_str().unwrap();
                    match val_str {
                        "TRUE" => {
                            values.insert(par_name.to_string(), Value::Bool(true));
                        }
                        "FALSE" => {
                            values.insert(par_name.to_string(), Value::Bool(false));
                        }
                        "nil" => {
                            values.insert(par_name.to_string(), Value::Null);
                        }
                        s => {
                            values.insert(par_name.to_string(), Value::Symbol(s.to_string()));
                        }
                    }
                }
                INTEGER_TYPE => {
                    values.insert(par_name.to_string(), Value::Int((*val.__bindgen_anon_1.integerValue).contents));
                }
                FLOAT_TYPE => {
                    values.insert(par_name.to_string(), Value::Float((*val.__bindgen_anon_1.floatValue).contents));
                }
                STRING_TYPE => {
                    let val_str = std::ffi::CStr::from_ptr((*val.__bindgen_anon_1.lexemeValue).contents).to_str().unwrap();
                    values.insert(par_name.to_string(), Value::String(val_str.to_string()));
                }
                _ => {}
            }
        }

        kb.add_data(object_id, values, time).unwrap();
    }
}

unsafe extern "C" fn add_class(_env: *mut Environment, udfc: *mut UDFContext, _out: *mut UDFValue) {
    unsafe {
        let kb = &*((*udfc).context as *mut CLIPSKnowledgeBase);
        let mut object_id = std::mem::MaybeUninit::<UDFValue>::uninit();
        if !UDFFirstArgument(udfc, CLIPSType_SYMBOL_BIT, object_id.as_mut_ptr()) {
            return;
        }
        let object_id = object_id.assume_init();
        if object_id.__bindgen_anon_1.header.is_null() {
            panic!("Received NULL header in UDF argument object_id in add_class");
        }
        let object_id = std::ffi::CStr::from_ptr((*object_id.__bindgen_anon_1.lexemeValue).contents).to_str().unwrap();

        let mut class_name = std::mem::MaybeUninit::<UDFValue>::uninit();
        if !UDFNextArgument(udfc, CLIPSType_SYMBOL_BIT, class_name.as_mut_ptr()) {
            return;
        }
        let class_name = class_name.assume_init();
        let class_name = std::ffi::CStr::from_ptr((*class_name.__bindgen_anon_1.lexemeValue).contents).to_str().unwrap();

        kb.add_class(object_id, class_name).unwrap();
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use std::collections::{HashMap, HashSet};
    use tokio::sync::broadcast;

    fn create_kb() -> Box<CLIPSKnowledgeBase> {
        let (tx, _) = broadcast::channel(100);
        let kb = Box::new(CLIPSKnowledgeBase::new(tx));
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
        let object = Object {
            id: Some("person1".to_string()),
            classes,
            properties: Some(HashMap::new()), // Empty map means use defaults/nulls as per logic
            values: None,
        };
        kb.create_object(&object).unwrap();

        // Update properties
        let mut new_props = HashMap::new();
        new_props.insert("age".to_string(), Value::Int(30));
        assert!(kb.set_properties(object.id.as_ref().unwrap(), new_props).is_ok());
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
        let object = Object { id: Some("sensor1".to_string()), classes, properties: None, values: Some(HashMap::new()) };
        kb.create_object(&object).unwrap();

        // Add data
        let mut values = HashMap::new();
        values.insert("temperature".to_string(), Value::Float(25.5));
        assert!(kb.add_data(object.id.as_ref().unwrap(), values, Utc::now()).is_ok());
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

    #[test]
    fn test_add_data_deadlock() {
        let kb = create_kb();

        let mut dynamic_props = HashMap::new();
        dynamic_props.insert("temp".to_string(), Property::Float { nullable: Some(false), default: Some(0.0), min: None, max: None });
        let class = Class {
            name: "Sensor".to_string(),
            parents: None,
            static_properties: None,
            dynamic_properties: Some(dynamic_props),
        };
        kb.create_class(&class).unwrap();

        let rule = Rule {
            name: "check-temp".to_string(),
            content: "(defrule check-temp (Sensor_temp (id ?id) (value ?v&:(> ?v 50.0))) => (add-data ?id (create$ temp) (create$ 0.0)))".to_string(),
        };
        kb.create_rule(&rule).unwrap();

        let mut classes = HashSet::new();
        classes.insert("Sensor".to_string());
        let object = Object { id: Some("s1".to_string()), classes, properties: None, values: Some(HashMap::new()) };
        kb.create_object(&object).unwrap();

        let mut values = HashMap::new();
        values.insert("temp".to_string(), Value::Float(100.0));
        kb.add_data("s1", values, Utc::now()).unwrap();

        assert!(kb.run().is_ok());

        let obj = kb.get_object("s1").unwrap();
        let (val, _) = obj.values.unwrap().get("temp").unwrap().clone();
        match val {
            Value::Float(f) => assert_eq!(f, 0.0),
            _ => panic!("Expected float"),
        }
    }

    #[test]
    fn test_add_class_deadlock() {
        let kb = create_kb();

        let mut static_props = HashMap::new();
        static_props.insert("age".to_string(), Property::Int { nullable: Some(false), default: Some(0), min: None, max: None });
        let person_class = Class {
            name: "Person".to_string(),
            parents: None,
            static_properties: Some(static_props),
            dynamic_properties: None,
        };
        kb.create_class(&person_class).unwrap();

        let adult_class = Class {
            name: "Adult".to_string(),
            parents: None,
            static_properties: None,
            dynamic_properties: None,
        };
        kb.create_class(&adult_class).unwrap();

        let rule = Rule {
            name: "check-age".to_string(),
            content: "(defrule check-age (Person_age (id ?id) (value ?v&:(> ?v 18))) => (add-class ?id Adult))".to_string(),
        };
        kb.create_rule(&rule).unwrap();

        let mut classes = HashSet::new();
        classes.insert("Person".to_string());
        let object = Object { id: Some("p1".to_string()), classes, properties: Some(HashMap::new()), values: None };
        kb.create_object(&object).unwrap();

        let mut props = HashMap::new();
        props.insert("age".to_string(), Value::Int(20));
        kb.set_properties("p1", props).unwrap();

        assert!(kb.run().is_ok());

        let obj = kb.get_object("p1").unwrap();
        assert!(obj.classes.contains("Adult"));
    }
}

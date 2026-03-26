use crate::{
    kb::{KnowledgeBaseError, KnowledgeBaseEvent},
    model::{Class, Object, Rule, Value},
};
use chrono::{DateTime, Utc};
use clips::{ClipsValue, Environment, Fact, Type};
use std::{cell::RefCell, collections::HashMap, rc::Rc};
use tracing::debug;

pub struct CLIPSKnowledgeBase {
    classes: HashMap<String, Class>,
    objects: HashMap<String, Object>,
    rules: HashMap<String, Rule>,
    instances: HashMap<String, HashMap<String, Fact>>,               // class name -> object id -> fact
    values: HashMap<String, HashMap<String, HashMap<String, Fact>>>, // class name -> object id -> property name -> fact
    env: Environment,
    callback: Rc<RefCell<Option<Box<dyn Fn(KnowledgeBaseEvent)>>>>,
}

impl CLIPSKnowledgeBase {
    pub fn new() -> Result<Self, KnowledgeBaseError> {
        let callback = Rc::new(RefCell::new(None));
        let mut kb = CLIPSKnowledgeBase {
            classes: HashMap::new(),
            objects: HashMap::new(),
            rules: HashMap::new(),
            instances: HashMap::new(),
            values: HashMap::new(),
            env: Environment::new().map_err(|e| KnowledgeBaseError::CreationError(e.to_string()))?,
            callback: callback.clone(),
        };
        kb.env
            .add_udf("add-data", None, 3, 4, vec![Type(Type::SYMBOL), Type(Type::MULTIFIELD), Type(Type::MULTIFIELD), Type(Type::INTEGER)], move |_env, ctx| {
                debug!("add-data called");
                let object_id = ctx.get_next_argument(Type(Type::SYMBOL)).expect("Failed to get object ID argument for add-data UDF");
                let object_id = if let ClipsValue::Symbol(s) = object_id { s } else { panic!("Expected symbol for object ID argument in add-data UDF") };
                let args = ctx.get_next_argument(Type(Type::MULTIFIELD)).expect("Failed to get args argument for add-data UDF");
                let args: Vec<String> = if let ClipsValue::Multifield(mf) = args {
                    mf.into_iter()
                        .map(|v| match v {
                            ClipsValue::Symbol(s) => s,
                            _ => panic!("Expected symbol, integer, or float in args multifield for add-data UDF"),
                        })
                        .collect()
                } else {
                    panic!("Expected multifield for args argument in add-data UDF");
                };
                let vals = ctx.get_next_argument(Type(Type::MULTIFIELD)).expect("Failed to get values argument for add-data UDF");
                let vals: Vec<Value> = if let ClipsValue::Multifield(mf) = vals {
                    mf.into_iter()
                        .map(|v| match v {
                            ClipsValue::Integer(i) => Value::Int(i),
                            ClipsValue::Float(f) => Value::Float(f),
                            ClipsValue::Symbol(s) => match s.as_str() {
                                "TRUE" => Value::Bool(true),
                                "FALSE" => Value::Bool(false),
                                "nil" => Value::Null,
                                other => Value::Symbol(other.to_owned()),
                            },
                            ClipsValue::String(s) => Value::String(s),
                            _ => panic!("Expected symbol, integer, or float in values multifield for add-data UDF"),
                        })
                        .collect()
                } else {
                    panic!("Expected multifield for values argument in add-data UDF");
                };
                let date_time = if ctx.has_next_argument() { Some(ctx.get_next_argument(Type(Type::INTEGER)).expect("Failed to get date_time argument for add-data UDF")) } else { None };
                let date_time = date_time
                    .map(|dt| {
                        let dt = if let ClipsValue::Integer(i) = dt { i } else { panic!("Expected integer for date_time argument in add-data UDF") };
                        DateTime::<Utc>::from_timestamp(dt, 0).expect("Failed to convert date_time argument in add-data UDF")
                    })
                    .unwrap_or(Utc::now());
                let values = args.into_iter().zip(vals).collect::<HashMap<_, _>>();
                if let Some(cb) = callback.borrow().as_ref() {
                    cb(KnowledgeBaseEvent::AddedValues(object_id.clone(), values.clone(), date_time));
                }
                ClipsValue::Void()
            })
            .expect("Failed to add CLIPS function");
        Ok(kb)
    }
}

impl crate::kb::KnowledgeBase for CLIPSKnowledgeBase {
    fn run(&mut self) -> Result<(), crate::kb::KnowledgeBaseError> {
        unimplemented!()
    }

    fn set_callback(&self, cb: impl Fn(KnowledgeBaseEvent) + 'static) {
        self.callback.replace(Some(Box::new(cb)));
    }
}

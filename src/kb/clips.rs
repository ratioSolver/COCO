use crate::{
    kb::{KnowledgeBase, KnowledgeBaseError, KnowledgeBaseEvent},
    model::{Class, Object, Rule, Value},
};
use async_trait::async_trait;
use chrono::{DateTime, Utc};
use clips::{ClipsValue, Environment, Fact, Type};
use std::{
    collections::HashMap,
    sync::{Arc, Mutex},
};
use tokio::sync::{mpsc, oneshot};
use tracing::{error, info, trace};

#[derive(Debug)]
enum KBCommand {
    GetClasses(oneshot::Sender<Result<Vec<Class>, KnowledgeBaseError>>),
    GetClass(String, oneshot::Sender<Result<Class, KnowledgeBaseError>>),
    CreateClass(Class, oneshot::Sender<Result<(), KnowledgeBaseError>>),
    GetRules(oneshot::Sender<Result<Vec<Rule>, KnowledgeBaseError>>),
    GetRule(String, oneshot::Sender<Result<Rule, KnowledgeBaseError>>),
    CreateRule(Rule, oneshot::Sender<Result<(), KnowledgeBaseError>>),
    CreateObject(Object, oneshot::Sender<Result<(), KnowledgeBaseError>>),
    AddClass(String, String, oneshot::Sender<Result<(), KnowledgeBaseError>>),
    SetProperties(String, HashMap<String, Value>, oneshot::Sender<Result<(), KnowledgeBaseError>>),
    AddValues(String, HashMap<String, Value>, DateTime<Utc>, oneshot::Sender<Result<(), KnowledgeBaseError>>),
    SetLLMResult(String, String, oneshot::Sender<Result<(), KnowledgeBaseError>>),
    Run(oneshot::Sender<Result<(), KnowledgeBaseError>>),
}

#[derive(Clone)]
pub struct CLIPSKnowledgeBase {
    tx: mpsc::Sender<KBCommand>,
    event_rx: Arc<Mutex<Option<mpsc::Receiver<KnowledgeBaseEvent>>>>,
}

struct ActorState {
    classes: HashMap<String, Class>,
    objects: HashMap<String, Object>,
    rules: HashMap<String, Rule>,

    env: Environment,
    instances: HashMap<String, HashMap<String, Fact>>,               // class name -> object id -> fact
    values: HashMap<String, HashMap<String, HashMap<String, Fact>>>, // class name -> object id -> property name -> fact
    llm_results: HashMap<String, (String, Fact)>,                    // object id -> (result, fact)
}

impl CLIPSKnowledgeBase {
    pub fn new() -> Self {
        let (tx, mut rx) = mpsc::channel(100);
        let (event_tx, event_rx) = mpsc::channel(100);

        tokio::task::spawn_blocking(move || {
            let env = Environment::new().expect("Failed to create CLIPS environment");
            let mut kb = ActorState {
                classes: HashMap::new(),
                objects: HashMap::new(),
                rules: HashMap::new(),
                env,
                instances: HashMap::new(),
                values: HashMap::new(),
                llm_results: HashMap::new(),
            };

            kb.env.build("(deftemplate llm-result (slot item_id (type SYMBOL)) (slot result (type STRING)))").expect("Failed to build CLIPS template");

            let add_data_event_tx = event_tx.clone();
            kb.env
                .add_udf("add-data", None, 3, 4, vec![Type(Type::SYMBOL), Type(Type::MULTIFIELD), Type(Type::MULTIFIELD), Type(Type::INTEGER)], move |_env, ctx| {
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

                    add_data_event_tx.blocking_send(KnowledgeBaseEvent::AddedValues(object_id.clone(), args.into_iter().zip(vals.into_iter()).collect(), date_time)).expect("Failed to send AddedValues event from add-data UDF");

                    ClipsValue::Void()
                })
                .expect("Failed to add CLIPS function");

            while let Some(cmd) = rx.blocking_recv() {
                match cmd {
                    KBCommand::GetClasses(reply) => {
                        trace!("Getting all classes");
                        let _ = reply.send(Ok(kb.classes.values().cloned().collect()));
                    }
                    KBCommand::GetClass(name, reply) => {
                        trace!("Getting class: {}", name);
                        let _ = reply.send(kb.classes.get(&name).cloned().ok_or(KnowledgeBaseError::ClassNotFound(name)));
                    }
                    KBCommand::CreateClass(class, reply) => {
                        trace!("Creating class: {}", class.name);
                        if kb.classes.contains_key(&class.name) {
                            let _ = reply.send(Err(KnowledgeBaseError::ClassAlreadyExists(class.name.clone())));
                            continue;
                        } else if let Some(parents) = &class.parents {
                            if let Some(missing_parent) = parents.iter().find(|p| !kb.classes.contains_key(*p)) {
                                let _ = reply.send(Err(KnowledgeBaseError::ClassNotFound(missing_parent.clone())));
                                continue;
                            }
                        }
                        let _ = reply.send(Ok(()));
                    }
                    KBCommand::GetRules(reply) => {
                        trace!("Getting all rules");
                        let _ = reply.send(Ok(kb.rules.values().cloned().collect()));
                    }
                    KBCommand::GetRule(name, reply) => {
                        trace!("Getting rule: {}", name);
                        let _ = reply.send(kb.rules.get(&name).cloned().ok_or(KnowledgeBaseError::RuleNotFound(name)));
                    }
                    KBCommand::CreateRule(rule, reply) => {
                        trace!("Creating rule: {}", rule.name);
                        let _ = reply.send(Ok(()));
                    }
                    KBCommand::CreateObject(object, reply) => {
                        if let Some(object_id) = object.id {
                            trace!("Creating object: {}", object_id);
                            if kb.objects.contains_key(&object_id) {
                                let _ = reply.send(Err(KnowledgeBaseError::ObjectAlreadyExists(object_id.clone())));
                                continue;
                            } else if let Some(missing_class) = object.classes.iter().find(|c| !kb.classes.contains_key(*c)) {
                                let _ = reply.send(Err(KnowledgeBaseError::ClassNotFound(missing_class.clone())));
                                continue;
                            }
                        } else {
                            let _ = reply.send(Err(KnowledgeBaseError::CreationError("Object ID is required".to_string())));
                            continue;
                        }
                        let _ = reply.send(Ok(()));
                    }
                    KBCommand::AddClass(object_id, class_name, reply) => {
                        trace!("Adding class '{}' to object '{}'", class_name, object_id);
                        let _ = event_tx.blocking_send(KnowledgeBaseEvent::AddedClass(object_id.clone(), class_name.clone()));
                        let _ = reply.send(Ok(()));
                    }
                    KBCommand::SetProperties(object_id, properties, reply) => {
                        trace!("Setting properties for object '{}': {:?}", object_id, properties);
                        let _ = event_tx.blocking_send(KnowledgeBaseEvent::UpdatedProperties(object_id.clone(), properties.clone()));
                        let _ = reply.send(Ok(()));
                    }
                    KBCommand::AddValues(object_id, values, timestamp, reply) => {
                        trace!("Adding values for object '{}': {:?} at {}", object_id, values, timestamp);
                        let _ = event_tx.blocking_send(KnowledgeBaseEvent::AddedValues(object_id.clone(), values.clone(), timestamp));
                        let _ = reply.send(Ok(()));
                    }
                    KBCommand::SetLLMResult(object_id, result, reply) => {
                        trace!("Setting LLM result for object '{}': {}", object_id, result);
                        let _ = reply.send(Ok(()));
                    }
                    KBCommand::Run(reply) => {
                        trace!("Running inference");
                        kb.env.run(-1);
                        let _ = reply.send(Ok(()));
                    }
                }
            }
        });

        Self { tx, event_rx: Arc::new(Mutex::new(Some(event_rx))) }
    }
}

#[async_trait]
impl KnowledgeBase for CLIPSKnowledgeBase {
    async fn create_class(&self, class: Class) -> Result<(), KnowledgeBaseError> {
        let (reply_tx, reply_rx) = oneshot::channel();
        self.tx.send(KBCommand::CreateClass(class, reply_tx)).await.map_err(|e| KnowledgeBaseError::KBError(format!("Failed to send CreateClass command: {}", e)))?;
        reply_rx.await.map_err(|e| KnowledgeBaseError::KBError(format!("Failed to receive response for CreateClass command: {}", e)))?
    }

    async fn create_rule(&self, rule: Rule) -> Result<(), KnowledgeBaseError> {
        let (reply_tx, reply_rx) = oneshot::channel();
        self.tx.send(KBCommand::CreateRule(rule, reply_tx)).await.map_err(|e| KnowledgeBaseError::KBError(format!("Failed to send CreateRule command: {}", e)))?;
        reply_rx.await.map_err(|e| KnowledgeBaseError::KBError(format!("Failed to receive response for CreateRule command: {}", e)))?
    }

    async fn create_object(&self, object: Object) -> Result<(), KnowledgeBaseError> {
        let (reply_tx, reply_rx) = oneshot::channel();
        self.tx.send(KBCommand::CreateObject(object, reply_tx)).await.map_err(|e| KnowledgeBaseError::KBError(format!("Failed to send CreateObject command: {}", e)))?;
        reply_rx.await.map_err(|e| KnowledgeBaseError::KBError(format!("Failed to receive response for CreateObject command: {}", e)))?
    }

    fn take_event_receiver(&mut self) -> Option<mpsc::Receiver<KnowledgeBaseEvent>> {
        let mut guard = self.event_rx.lock().unwrap();
        guard.take()
    }
}

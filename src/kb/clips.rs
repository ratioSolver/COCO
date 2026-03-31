use crate::{
    kb::{KBCommand, KnowledgeBase, KnowledgeBaseError, KnowledgeBaseEvent},
    model::{Class, Object, Rule},
};
use async_trait::async_trait;
use clips::{Environment, Fact};
use std::{
    collections::HashMap,
    sync::{Arc, Mutex},
};
use tokio::sync::mpsc;
use tracing::{error, info, trace};

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
            let mut state = ActorState {
                classes: HashMap::new(),
                objects: HashMap::new(),
                rules: HashMap::new(),
                env,
                instances: HashMap::new(),
                values: HashMap::new(),
                llm_results: HashMap::new(),
            };

            while let Some(cmd) = rx.blocking_recv() {
                match cmd {
                    KBCommand::GetClasses(reply) => {
                        trace!("Getting all classes");
                        let _ = reply.send(Ok(state.classes.values().cloned().collect()));
                    }
                    KBCommand::GetClass(name, reply) => {
                        trace!("Getting class: {}", name);
                        let _ = reply.send(state.classes.get(&name).cloned().ok_or(KnowledgeBaseError::ClassNotFound(name)));
                    }
                    KBCommand::CreateClass(class, reply) => {
                        trace!("Creating class: {}", class.name);
                        if state.classes.contains_key(&class.name) {
                            let _ = reply.send(Err(KnowledgeBaseError::ClassAlreadyExists(class.name.clone())));
                            continue;
                        } else if let Some(parents) = &class.parents {
                            if let Some(missing_parent) = parents.iter().find(|p| !state.classes.contains_key(*p)) {
                                let _ = reply.send(Err(KnowledgeBaseError::ClassNotFound(missing_parent.clone())));
                                continue;
                            }
                        }
                        let _ = reply.send(Ok(()));
                    }
                    KBCommand::GetRules(reply) => {
                        trace!("Getting all rules");
                        let _ = reply.send(Ok(state.rules.values().cloned().collect()));
                    }
                    KBCommand::GetRule(name, reply) => {
                        trace!("Getting rule: {}", name);
                        let _ = reply.send(state.rules.get(&name).cloned().ok_or(KnowledgeBaseError::RuleNotFound(name)));
                    }
                    KBCommand::CreateRule(rule, reply) => {
                        trace!("Creating rule: {}", rule.name);
                        let _ = reply.send(Ok(()));
                    }
                    KBCommand::CreateObject(object, reply) => {
                        if let Some(object_id) = object.id {
                            trace!("Creating object: {}", object_id);
                            if state.objects.contains_key(&object_id) {
                                let _ = reply.send(Err(KnowledgeBaseError::ObjectAlreadyExists(object_id.clone())));
                                continue;
                            } else if let Some(missing_class) = object.classes.iter().find(|c| !state.classes.contains_key(*c)) {
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
                        state.env.run(-1);
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
    async fn send_command(&self, cmd: KBCommand) -> Result<(), KnowledgeBaseError> {
        self.tx.send(cmd).await.map_err(|e| KnowledgeBaseError::KBError(e.to_string()))
    }

    fn take_event_receiver(&mut self) -> Option<mpsc::Receiver<KnowledgeBaseEvent>> {
        let mut guard = self.event_rx.lock().unwrap();
        guard.take()
    }
}

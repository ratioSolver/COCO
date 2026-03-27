use crate::{
    kb::{KnowledgeBase, KnowledgeBaseError, KnowledgeBaseEvent},
    model::{Class, Object, Rule, TimedValue, Value},
};
use chrono::{DateTime, Utc};
use clips::Environment;
use std::{
    collections::HashMap,
    sync::{Arc, RwLock, mpsc},
    thread,
};

pub type Callback = Arc<dyn Fn(KnowledgeBaseEvent) + Send + Sync + 'static>;

type Reply<T> = mpsc::Sender<Result<T, KnowledgeBaseError>>;

enum Command {
    CreateClass(Class, Reply<()>),
    CreateObject(Object, Reply<()>),
    AddClass(String, String, Reply<()>),
    SetProperties(String, HashMap<String, Value>, Reply<()>),
    AddValues(String, HashMap<String, Value>, DateTime<Utc>, Reply<()>),
    CreateRule(Rule, Reply<()>),
    Run(Reply<()>),
    SetCallback(Callback),
}

struct ActorState {
    classes: HashMap<String, Class>,
    objects: HashMap<String, Object>,
    rules: HashMap<String, Rule>,
    env: Option<Environment>,
    callback: Option<Callback>,
}

impl ActorState {
    fn new() -> Self {
        Self {
            classes: HashMap::new(),
            objects: HashMap::new(),
            rules: HashMap::new(),
            env: None,
            callback: None,
        }
    }

    fn env_mut(&mut self) -> Result<&mut Environment, KnowledgeBaseError> {
        if self.env.is_none() {
            let env = Environment::new().map_err(|e| KnowledgeBaseError::CreationError(format!("Failed to create CLIPS environment: {}", e)))?;
            self.env = Some(env);
        }
        self.env.as_mut().ok_or_else(|| KnowledgeBaseError::CreationError("CLIPS environment not initialized".to_owned()))
    }

    fn create_class(&mut self, class: Class) -> Result<(), KnowledgeBaseError> {
        let class_name = class.name.clone();
        if self.classes.contains_key(&class_name) {
            return Err(KnowledgeBaseError::ClassAlreadyExists(class_name));
        }
        self.env_mut()?.build(format!("(deftemplate {} (slot id (type SYMBOL)))", class.name).as_str()).map_err(|e| KnowledgeBaseError::KBError(format!("Failed to create class in CLIPS: {}", e)))?;
        self.classes.insert(class_name, class);
        Ok(())
    }

    fn create_object(&mut self, object: Object) -> Result<(), KnowledgeBaseError> {
        let id = object.id.clone().ok_or_else(|| KnowledgeBaseError::ObjectNotFound("Object must have an ID".to_owned()))?;
        if self.objects.contains_key(&id) {
            return Err(KnowledgeBaseError::ObjectAlreadyExists(id));
        }
        for class_name in &object.classes {
            if !self.classes.contains_key(class_name) {
                return Err(KnowledgeBaseError::ClassNotFound(class_name.clone()));
            }
        }
        self.objects.insert(id, object);
        Ok(())
    }

    fn add_class(&mut self, object_id: &str, class_name: &str) -> Result<(), KnowledgeBaseError> {
        if !self.classes.contains_key(class_name) {
            return Err(KnowledgeBaseError::ClassNotFound(class_name.to_owned()));
        }
        let object = self.objects.get_mut(object_id).ok_or_else(|| KnowledgeBaseError::ObjectNotFound(object_id.to_owned()))?;
        object.classes.insert(class_name.to_owned());
        if let Some(cb) = &self.callback {
            cb(KnowledgeBaseEvent::AddedClass(object_id.to_owned(), class_name.to_owned()));
        }
        Ok(())
    }

    fn set_properties(&mut self, object_id: &str, properties: HashMap<String, Value>) -> Result<(), KnowledgeBaseError> {
        let object = self.objects.get_mut(object_id).ok_or_else(|| KnowledgeBaseError::ObjectNotFound(object_id.to_owned()))?;
        let object_props = object.properties.get_or_insert_with(HashMap::new);
        for (k, v) in &properties {
            object_props.insert(k.clone(), v.clone());
        }
        if let Some(cb) = &self.callback {
            cb(KnowledgeBaseEvent::UpdatedProperties(object_id.to_owned(), properties));
        }
        Ok(())
    }

    fn add_values(&mut self, object_id: &str, values: HashMap<String, Value>, date_time: DateTime<Utc>) -> Result<(), KnowledgeBaseError> {
        let object = self.objects.get_mut(object_id).ok_or_else(|| KnowledgeBaseError::ObjectNotFound(object_id.to_owned()))?;
        let object_vals = object.values.get_or_insert_with(HashMap::new);
        for (k, v) in &values {
            object_vals.insert(k.clone(), TimedValue { value: v.clone(), timestamp: date_time });
        }
        if let Some(cb) = &self.callback {
            cb(KnowledgeBaseEvent::AddedValues(object_id.to_owned(), values, date_time));
        }
        Ok(())
    }

    fn create_rule(&mut self, rule: Rule) -> Result<(), KnowledgeBaseError> {
        let rule_name = rule.name.clone();
        if self.rules.contains_key(&rule_name) {
            return Err(KnowledgeBaseError::RuleAlreadyExists(rule_name));
        }
        self.env_mut()?.build(rule.content.as_str()).map_err(|e| KnowledgeBaseError::KBError(format!("Failed to create rule in CLIPS: {}", e)))?;
        self.rules.insert(rule.name.clone(), rule);
        Ok(())
    }

    fn run(&mut self) -> Result<(), KnowledgeBaseError> {
        self.env_mut()?.run(-1);
        Ok(())
    }
}

pub struct CLIPSKnowledgeBase {
    classes: HashMap<String, Class>,
    objects: HashMap<String, Object>,
    rules: HashMap<String, Rule>,
    callback: Arc<RwLock<Option<Callback>>>,
    tx: mpsc::Sender<Command>,
}

impl CLIPSKnowledgeBase {
    pub fn new() -> Result<Self, KnowledgeBaseError> {
        let (tx, rx) = mpsc::channel::<Command>();
        thread::spawn(move || {
            let mut state = ActorState::new();
            while let Ok(command) = rx.recv() {
                match command {
                    Command::CreateClass(class, reply) => {
                        let _ = reply.send(state.create_class(class));
                    }
                    Command::CreateObject(object, reply) => {
                        let _ = reply.send(state.create_object(object));
                    }
                    Command::AddClass(object_id, class_name, reply) => {
                        let _ = reply.send(state.add_class(&object_id, &class_name));
                    }
                    Command::SetProperties(object_id, properties, reply) => {
                        let _ = reply.send(state.set_properties(&object_id, properties));
                    }
                    Command::AddValues(object_id, values, date_time, reply) => {
                        let _ = reply.send(state.add_values(&object_id, values, date_time));
                    }
                    Command::CreateRule(rule, reply) => {
                        let _ = reply.send(state.create_rule(rule));
                    }
                    Command::Run(reply) => {
                        let _ = reply.send(state.run());
                    }
                    Command::SetCallback(cb) => {
                        state.callback = Some(cb);
                    }
                }
            }
        });

        Ok(Self {
            classes: HashMap::new(),
            objects: HashMap::new(),
            rules: HashMap::new(),
            callback: Arc::new(RwLock::new(None)),
            tx,
        })
    }

    fn call<T>(&self, command: impl FnOnce(Reply<T>) -> Command) -> Result<T, KnowledgeBaseError> {
        let (reply_tx, reply_rx) = mpsc::channel();
        self.tx.send(command(reply_tx)).map_err(|e| KnowledgeBaseError::KBError(format!("Failed to send command to CLIPS actor: {}", e)))?;
        reply_rx.recv().map_err(|e| KnowledgeBaseError::KBError(format!("Failed to receive reply from CLIPS actor: {}", e)))?
    }
}

impl KnowledgeBase for CLIPSKnowledgeBase {
    fn get_classes(&self) -> Vec<&Class> {
        self.classes.values().collect()
    }
    fn get_class(&self, name: &str) -> Option<&Class> {
        self.classes.get(name)
    }
    fn create_class(&mut self, class: Class) -> Result<(), KnowledgeBaseError> {
        let class_name = class.name.clone();
        self.call(|reply| Command::CreateClass(class.clone(), reply))?;
        self.classes.insert(class_name, class);
        Ok(())
    }

    fn get_objects(&self) -> Vec<&Object> {
        self.objects.values().collect()
    }
    fn get_object(&self, id: &str) -> Option<&Object> {
        self.objects.get(id)
    }
    fn create_object(&mut self, object: Object) -> Result<(), KnowledgeBaseError> {
        let id = object.id.clone().ok_or_else(|| KnowledgeBaseError::ObjectNotFound("Object must have an ID".to_owned()))?;
        self.call(|reply| Command::CreateObject(object.clone(), reply))?;
        self.objects.insert(id, object);
        Ok(())
    }
    fn add_class(&mut self, object_id: &str, class_name: &str) -> Result<(), KnowledgeBaseError> {
        self.call(|reply| Command::AddClass(object_id.to_owned(), class_name.to_owned(), reply))?;
        let object = self.objects.get_mut(object_id).ok_or_else(|| KnowledgeBaseError::ObjectNotFound(object_id.to_owned()))?;
        object.classes.insert(class_name.to_owned());
        Ok(())
    }
    fn set_properties(&mut self, object_id: &str, properties: HashMap<String, Value>) -> Result<(), KnowledgeBaseError> {
        self.call(|reply| Command::SetProperties(object_id.to_owned(), properties.clone(), reply))?;
        let object = self.objects.get_mut(object_id).ok_or_else(|| KnowledgeBaseError::ObjectNotFound(object_id.to_owned()))?;
        let object_props = object.properties.get_or_insert_with(HashMap::new);
        for (k, v) in properties {
            object_props.insert(k, v);
        }
        Ok(())
    }
    fn add_values(&mut self, object_id: &str, values: HashMap<String, Value>, date_time: DateTime<Utc>) -> Result<(), KnowledgeBaseError> {
        self.call(|reply| Command::AddValues(object_id.to_owned(), values.clone(), date_time, reply))?;
        let object = self.objects.get_mut(object_id).ok_or_else(|| KnowledgeBaseError::ObjectNotFound(object_id.to_owned()))?;
        let object_vals = object.values.get_or_insert_with(HashMap::new);
        for (k, v) in values {
            object_vals.insert(k, TimedValue { value: v, timestamp: date_time });
        }
        Ok(())
    }

    fn get_rules(&self) -> Vec<&Rule> {
        self.rules.values().collect()
    }
    fn get_rule(&self, name: &str) -> Option<&Rule> {
        self.rules.get(name)
    }
    fn create_rule(&mut self, rule: Rule) -> Result<(), KnowledgeBaseError> {
        let rule_name = rule.name.clone();
        self.call(|reply| Command::CreateRule(rule.clone(), reply))?;
        self.rules.insert(rule_name, rule);
        Ok(())
    }

    fn run(&mut self) -> Result<(), KnowledgeBaseError> {
        self.call(Command::Run)
    }

    fn set_callback(&self, cb: impl Fn(KnowledgeBaseEvent) + Send + Sync + 'static) {
        let callback: Callback = Arc::new(cb);
        if let Ok(mut guard) = self.callback.write() {
            *guard = Some(callback.clone());
        }
        let _ = self.tx.send(Command::SetCallback(callback));
    }
}

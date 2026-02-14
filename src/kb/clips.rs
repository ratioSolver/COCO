use crate::{
    kb::{KnowledgeBase, KnowledgeBaseError},
    model::{Class, CoCoEvent, Object, Property, Rule, Value},
};
use chrono::{DateTime, Utc};
use clips::{ClipsValue, Environment, Fact, FactBuilder, FactModifier, Type};
use std::collections::HashMap;
use tokio::sync::broadcast;

pub struct CLIPSKnowledgeBase {
    sender: broadcast::Sender<CoCoEvent>,
    classes: HashMap<String, Class>,
    objects: HashMap<String, Object>,
    rules: HashMap<String, Rule>,
    instances: HashMap<String, HashMap<String, Fact>>,               // class name -> object id -> fact
    values: HashMap<String, HashMap<String, HashMap<String, Fact>>>, // class name -> object id -> property name -> fact
    env: Environment,
}

unsafe impl Send for CLIPSKnowledgeBase {}
unsafe impl Sync for CLIPSKnowledgeBase {}

impl Default for CLIPSKnowledgeBase {
    fn default() -> Self {
        Self::new()
    }
}

impl CLIPSKnowledgeBase {
    pub fn new() -> Self {
        let (sender, _receiver) = broadcast::channel(16);
        let mut kb = Self {
            sender: sender.clone(),
            classes: HashMap::new(),
            objects: HashMap::new(),
            rules: HashMap::new(),
            instances: HashMap::new(),
            values: HashMap::new(),
            env: Environment::new().expect("Failed to create CLIPS environment"),
        };
        {
            let add_data_sender = sender.clone();
            kb.env
                .add_udf("add-data", None, 3, 4, vec![Type(Type::SYMBOL), Type(Type::MULTIFIELD), Type(Type::MULTIFIELD), Type(Type::INTEGER)], move |_env, ctx| {
                    println!("add-data UDF called with {} arguments", ctx.has_next_argument());
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
                                    other => Value::Symbol(other.to_string()),
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
                    let _ = add_data_sender.send(CoCoEvent::PendingValues(object_id, values, date_time));
                    ClipsValue::Void()
                })
                .expect("Failed to add UDF to CLIPS environment");
            let add_class_sender = sender.clone();
            kb.env
                .add_udf("add-class", None, 2, 2, vec![Type(Type::SYMBOL), Type(Type::SYMBOL)], move |_env, ctx| {
                    let object_id = ctx.get_next_argument(Type(Type::SYMBOL)).expect("Failed to get object ID argument for add-class UDF");
                    let object_id = if let ClipsValue::Symbol(s) = object_id { s } else { panic!("Expected symbol for object ID argument in add-class UDF") };
                    let class_name = ctx.get_next_argument(Type(Type::SYMBOL)).expect("Failed to get class name argument for add-class UDF");
                    let class_name = if let ClipsValue::Symbol(s) = class_name { s } else { panic!("Expected symbol for class name argument in add-class UDF") };
                    let _ = add_class_sender.send(CoCoEvent::PendingClass(object_id, class_name));
                    ClipsValue::Void()
                })
                .expect("Failed to add UDF to CLIPS environment");
            let prompt_sender = sender.clone();
            kb.env
                .add_udf("prompt", None, 2, 2, vec![Type(Type::SYMBOL), Type(Type::STRING)], move |_env, ctx| {
                    let object_id = ctx.get_next_argument(Type(Type::SYMBOL)).expect("Failed to get object ID argument for prompt UDF");
                    let object_id = if let ClipsValue::Symbol(s) = object_id { s } else { panic!("Expected symbol for object ID argument in prompt UDF") };
                    let message = ctx.get_next_argument(Type(Type::STRING)).expect("Failed to get message argument for prompt UDF");
                    let message = if let ClipsValue::String(s) = message { s } else { panic!("Expected string for message argument in prompt UDF") };
                    let _ = prompt_sender.send(CoCoEvent::LLMPrompt(object_id, message));
                    ClipsValue::Void()
                })
                .expect("Failed to add UDF to CLIPS environment");
            let fcm_sender = sender.clone();
            kb.env
                .add_udf("fcm", None, 3, 3, vec![Type(Type::SYMBOL), Type(Type::STRING), Type(Type::STRING)], move |_env, ctx| {
                    let object_id = ctx.get_next_argument(Type(Type::SYMBOL)).expect("Failed to get object ID argument for fcm UDF");
                    let object_id = if let ClipsValue::Symbol(s) = object_id { s } else { panic!("Expected symbol for object ID argument in fcm UDF") };
                    let title = ctx.get_next_argument(Type(Type::STRING)).expect("Failed to get title argument for fcm UDF");
                    let title = if let ClipsValue::String(s) = title { s } else { panic!("Expected string for title argument in fcm UDF") };
                    let message = ctx.get_next_argument(Type(Type::STRING)).expect("Failed to get message argument for fcm UDF");
                    let message = if let ClipsValue::String(s) = message { s } else { panic!("Expected string for message argument in fcm UDF") };
                    let _ = fcm_sender.send(CoCoEvent::FCMMessage(object_id, title, message));
                    ClipsValue::Void()
                })
                .expect("Failed to add UDF to CLIPS environment");
        }
        kb
    }
}

impl KnowledgeBase for CLIPSKnowledgeBase {
    fn get_event_sender(&self) -> broadcast::Sender<CoCoEvent> {
        self.sender.clone()
    }

    fn get_classes(&self) -> Vec<Class> {
        self.classes.values().cloned().collect()
    }

    fn get_class(&self, name: &str) -> Option<Class> {
        self.classes.get(name).cloned()
    }

    fn create_class(&mut self, class: Class) -> Result<(), KnowledgeBaseError> {
        let class_name = class.name.clone();
        if self.classes.contains_key(&class_name) {
            return Err(KnowledgeBaseError::ClassAlreadyExists(class.name.clone()));
        }
        self.env.build(format!("(deftemplate {} (slot id (type SYMBOL)))", class.name).as_str()).map_err(|e| KnowledgeBaseError::KBError(format!("Failed to create class in CLIPS: {}", e)))?;
        if let Some(static_props) = &class.static_properties {
            for (name, prop) in static_props {
                self.env.build(prop_deftemplate(&class, name, prop, true).as_str()).map_err(|e| KnowledgeBaseError::KBError(format!("Failed to create static property {} for class {} in CLIPS: {}", name, class.name, e)))?;
            }
        }
        if let Some(dynamic_props) = &class.dynamic_properties {
            for (name, prop) in dynamic_props {
                self.env.build(prop_deftemplate(&class, name, prop, false).as_str()).map_err(|e| KnowledgeBaseError::KBError(format!("Failed to create dynamic property {} for class {} in CLIPS: {}", name, class.name, e)))?;
            }
        }
        self.classes.insert(class_name.clone(), class);
        let _ = self.sender.send(CoCoEvent::ClassCreated(class_name));
        Ok(())
    }

    fn get_objects(&self) -> Vec<Object> {
        self.objects.values().cloned().collect()
    }

    fn get_object(&self, id: &str) -> Option<Object> {
        self.objects.get(id).cloned()
    }

    fn create_object(&mut self, object: Object) -> Result<(), KnowledgeBaseError> {
        let id = object.id.clone().ok_or_else(|| KnowledgeBaseError::ObjectNotFound("Object must have an ID".to_string()))?;
        if self.objects.contains_key(&id) {
            return Err(KnowledgeBaseError::ObjectAlreadyExists(id.clone()));
        }
        for class_name in object.classes.iter() {
            if let Some(class) = self.classes.get(class_name.as_str()) {
                let fb = self.env.fact_builder(&class.name).map_err(|e| KnowledgeBaseError::KBError(format!("Failed to create fact builder for class {}: {}", class_name, e)))?;
                let fb = fb.put_symbol("id", id.as_str()).map_err(|e| KnowledgeBaseError::KBError(format!("Failed to set id slot for object {}: {}", id, e)))?;
                let fact = self.env.assert_fact(fb).map_err(|e| KnowledgeBaseError::KBError(format!("Failed to assert fact for object {}: {}", id, e)))?;
                self.instances.entry(class.name.clone()).or_default().insert(id.clone(), fact);

                if let Some(static_props) = &class.static_properties {
                    for (name, prop) in static_props {
                        let fb = self.env.fact_builder(&format!("{}_{}", class.name, name)).map_err(|e| KnowledgeBaseError::KBError(format!("Failed to create fact builder for property {} of object {}: {}", name, id, e)))?;
                        let fb = fb.put_symbol("id", id.as_str()).map_err(|e| KnowledgeBaseError::KBError(format!("Failed to set id slot for property {} of object {}: {}", name, id, e)))?;
                        if let Some(v) = object.properties.as_ref().and_then(|props| props.get(name)) {
                            let fb: FactBuilder = set_prop(&self.env, fb, prop, v.clone(), None).map_err(|e| KnowledgeBaseError::KBError(format!("Failed to set property {} for object {}: {:#?}", name, id, e)))?;
                            let fact = self.env.assert_fact(fb).map_err(|e| KnowledgeBaseError::KBError(format!("Failed to assert fact for property {} of object {}: {}", name, id, e)))?;
                            self.values.entry(class.name.clone()).or_default().entry(id.clone()).or_default().insert(name.clone(), fact);
                        } else if let Some(def) = get_default(prop) {
                            let fb = set_prop(&self.env, fb, prop, def.clone(), None).map_err(|e| KnowledgeBaseError::KBError(format!("Failed to set default value for property {} of object {}: {:#?}", name, id, e)))?;
                            let fact = self.env.assert_fact(fb).map_err(|e| KnowledgeBaseError::KBError(format!("Failed to assert fact for default value of property {} of object {}: {}", name, id, e)))?;
                            self.values.entry(class.name.clone()).or_default().entry(id.clone()).or_default().insert(name.clone(), fact);
                        }
                    }
                }

                if let Some(dynamic_props) = &class.dynamic_properties {
                    for (name, prop) in dynamic_props {
                        let fb = self.env.fact_builder(&format!("{}_{}", class.name, name)).map_err(|e| KnowledgeBaseError::KBError(format!("Failed to create fact builder for dynamic property {} of object {}: {}", name, id, e)))?;
                        let fb = fb.put_symbol("id", id.as_str()).map_err(|e| KnowledgeBaseError::KBError(format!("Failed to set id slot for dynamic property {} of object {}: {}", name, id, e)))?;
                        if let Some(v) = object.values.as_ref().and_then(|vals| vals.get(name)) {
                            let fb = set_prop(&self.env, fb, prop, v.0.clone(), Some(v.1)).map_err(|e| KnowledgeBaseError::KBError(format!("Failed to set dynamic property {} for object {}: {:#?}", name, id, e)))?;
                            let fact = self.env.assert_fact(fb).map_err(|e| KnowledgeBaseError::KBError(format!("Failed to assert fact for dynamic property {} of object {}: {}", name, id, e)))?;
                            println!("Asserted fact for dynamic property {} of object {}: {}", name, id, fact);
                            self.values.entry(class.name.clone()).or_default().entry(id.clone()).or_default().insert(name.clone(), fact);
                        } else if let Some(def) = get_default(prop) {
                            let fb = set_prop(&self.env, fb, prop, def.clone(), Some(Utc::now())).map_err(|e| KnowledgeBaseError::KBError(format!("Failed to set default value for dynamic property {} of object {}: {:#?}", name, id, e)))?;
                            let fact = self.env.assert_fact(fb).map_err(|e| KnowledgeBaseError::KBError(format!("Failed to assert fact for default value of dynamic property {} of object {}: {}", name, id, e)))?;
                            self.values.entry(class.name.clone()).or_default().entry(id.clone()).or_default().insert(name.clone(), fact);
                        }
                    }
                }
            } else {
                return Err(KnowledgeBaseError::ClassNotFound(format!("Class {} not found for object {}", class_name, id)));
            }
        }
        self.objects.insert(id.clone(), object);
        let _ = self.sender.send(CoCoEvent::ObjectCreated(id));
        Ok(())
    }

    fn add_class(&mut self, object_id: &str, class_name: &str) -> Result<(), KnowledgeBaseError> {
        let object = self.objects.get_mut(object_id).ok_or_else(|| KnowledgeBaseError::ObjectNotFound(object_id.to_string()))?;
        let class = self.classes.get(class_name).ok_or_else(|| KnowledgeBaseError::ClassNotFound(class_name.to_string()))?;
        let fb = self.env.fact_builder(&class.name).unwrap().put_symbol("id", object_id).map_err(|e| KnowledgeBaseError::KBError(format!("Failed to set id slot for object {}: {}", object_id, e)))?;
        let fact = self.env.assert_fact(fb).map_err(|e| KnowledgeBaseError::KBError(format!("Failed to assert fact for object {}: {}", object_id, e)))?;
        self.instances.entry(class.name.clone()).or_default().insert(object_id.to_string(), fact);

        if let Some(static_props) = &class.static_properties {
            for (name, prop) in static_props {
                let fb = self.env.fact_builder(&format!("{}_{}", class.name, name)).map_err(|e| KnowledgeBaseError::KBError(format!("Failed to create fact builder for property {} of object {}: {}", name, object_id, e)))?;
                if let Some(v) = object.properties.as_ref().and_then(|props| props.get(name)) {
                    let fb = set_prop(&self.env, fb, prop, v.clone(), None)?;
                    let fact = self.env.assert_fact(fb).map_err(|e| KnowledgeBaseError::KBError(format!("Failed to assert fact for property {} of object {}: {}", name, object_id, e)))?;
                    self.values.entry(class.name.clone()).or_default().entry(object_id.to_string()).or_default().insert(name.clone(), fact);
                } else if let Some(def) = get_default(prop) {
                    let fb = set_prop(&self.env, fb, prop, def.clone(), None)?;
                    let fact = self.env.assert_fact(fb).map_err(|e| KnowledgeBaseError::KBError(format!("Failed to assert fact for default value of property {} of object {}: {}", name, object_id, e)))?;
                    self.values.entry(class.name.clone()).or_default().entry(object_id.to_string()).or_default().insert(name.clone(), fact);
                }
            }
        }

        if let Some(dynamic_props) = &class.dynamic_properties {
            for (name, prop) in dynamic_props {
                if let Some(v) = object.values.as_ref().and_then(|vals| vals.get(name)) {
                    let fb = self.env.fact_builder(&format!("{}_{}", class.name, name)).map_err(|e| KnowledgeBaseError::KBError(format!("Failed to create fact builder for dynamic property {} of object {}: {}", name, object_id, e)))?;
                    let fb = set_prop(&self.env, fb, prop, v.0.clone(), Some(v.1))?;
                    let fact = self.env.assert_fact(fb).map_err(|e| KnowledgeBaseError::KBError(format!("Failed to assert fact for dynamic property {} of object {}: {}", name, object_id, e)))?;
                    self.values.entry(class.name.clone()).or_default().entry(object_id.to_string()).or_default().insert(name.clone(), fact);
                } else if let Some(def) = get_default(prop) {
                    let fb = self.env.fact_builder(&format!("{}_{}", class.name, name)).map_err(|e| KnowledgeBaseError::KBError(format!("Failed to create fact builder for dynamic property {} of object {}: {}", name, object_id, e)))?;
                    let fb = set_prop(&self.env, fb, prop, def.clone(), Some(Utc::now()))?;
                    let fact = self.env.assert_fact(fb).map_err(|e| KnowledgeBaseError::KBError(format!("Failed to assert fact for default value of dynamic property {} of object {}: {}", name, object_id, e)))?;
                    self.values.entry(class.name.clone()).or_default().entry(object_id.to_string()).or_default().insert(name.clone(), fact);
                }
            }
        }
        let _ = self.sender.send(CoCoEvent::AddedClass(object_id.to_string(), class_name.to_string()));
        Ok(())
    }

    fn set_properties(&mut self, object_id: &str, properties: HashMap<String, Value>) -> Result<(), KnowledgeBaseError> {
        let object = self.objects.get_mut(object_id).ok_or_else(|| KnowledgeBaseError::ObjectNotFound(object_id.to_string()))?;
        for class_name in &object.classes {
            if let Some(class) = self.classes.get(class_name) {
                if let Some(static_props) = &class.static_properties {
                    for (name, prop) in static_props {
                        if let Some(v) = properties.get(name) {
                            let fact = self.values.get(class_name).and_then(|objs| objs.get(object_id)).and_then(|props| props.get(name)).ok_or_else(|| KnowledgeBaseError::ObjectNotFound(format!("Fact for property {} of object {} of class {} not found", name, object_id, class_name)))?;
                            let fm = self.env.fact_modifier(fact).map_err(|e| KnowledgeBaseError::KBError(format!("Failed to create fact modifier for object {}: {}", object_id, e)))?;
                            let fm = update_prop(&self.env, fm, prop, v.clone(), None)?;
                            self.env.modify_fact(fm).map_err(|e| KnowledgeBaseError::KBError(format!("Failed to modify fact for property {} of object {}: {}", name, object_id, e)))?;
                        }
                    }
                }
            } else {
                return Err(KnowledgeBaseError::ClassNotFound(format!("Class {} not found for object {}", class_name, object_id)));
            }
        }
        let _ = self.sender.send(CoCoEvent::UpdatedProperties(object_id.to_string(), properties));
        Ok(())
    }

    fn add_values(&mut self, object_id: &str, values: HashMap<String, Value>, date_time: DateTime<Utc>) -> Result<(), KnowledgeBaseError> {
        let object = self.objects.get_mut(object_id).ok_or_else(|| KnowledgeBaseError::ObjectNotFound(object_id.to_string()))?;
        for class_name in &object.classes {
            if let Some(class) = self.classes.get(class_name) {
                if let Some(dynamic_props) = &class.dynamic_properties {
                    for (name, prop) in dynamic_props {
                        if let Some(v) = values.get(name) {
                            let fact = self.values.get(class_name).and_then(|objs| objs.get(object_id)).and_then(|props| props.get(name)).ok_or_else(|| KnowledgeBaseError::ObjectNotFound(format!("Fact for dynamic property {} of object {} of class {} not found", name, object_id, class_name)))?;
                            let fm = self.env.fact_modifier(fact).map_err(|e| KnowledgeBaseError::KBError(format!("Failed to create fact modifier for object {}: {}", object_id, e)))?;
                            let fm = update_prop(&self.env, fm, prop, v.clone(), Some(date_time))?;
                            self.env.modify_fact(fm).map_err(|e| KnowledgeBaseError::KBError(format!("Failed to modify fact for dynamic property {} of object {}: {}", name, object_id, e)))?;
                        }
                    }
                }
            } else {
                return Err(KnowledgeBaseError::ClassNotFound(format!("Class {} not found for object {}", class_name, object_id)));
            }
        }
        if self.sender.receiver_count() > 0 {
            let _ = self.sender.send(CoCoEvent::AddedValues(object_id.to_string(), values, date_time));
        }
        Ok(())
    }

    fn get_rules(&self) -> Vec<Rule> {
        self.rules.values().cloned().collect()
    }

    fn get_rule(&self, name: &str) -> Option<Rule> {
        self.rules.get(name).cloned()
    }

    fn create_rule(&mut self, rule: Rule) -> Result<(), KnowledgeBaseError> {
        let rule_name = rule.name.clone();
        if self.rules.contains_key(&rule_name) {
            return Err(KnowledgeBaseError::RuleAlreadyExists(rule_name.clone()));
        }
        self.env.build(rule.content.as_str()).map_err(|e| KnowledgeBaseError::KBError(format!("Failed to create rule in CLIPS: {}", e)))?;

        self.rules.insert(rule_name.clone(), rule);
        let _ = self.sender.send(CoCoEvent::RuleCreated(rule_name));
        Ok(())
    }

    fn run(&mut self) -> Result<(), KnowledgeBaseError> {
        self.env.run(-1);
        Ok(())
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
        Property::BoolArray { nullable, default } => {
            def.push_str(" (multislot value (type SYMBOL) (allowed-symbols TRUE FALSE");
            if let Some(true) = nullable {
                def.push_str(" nil");
            }
            def.push(')');
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
        Property::IntArray { nullable, default, min, max } => {
            def.push_str(" (multislot value (type INTEGER");
            if let Some(true) = nullable {
                def.push_str(" SYMBOL) (allowed-symbols nil");
            }
            def.push(')');
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
        Property::FloatArray { nullable, default, min, max } => {
            def.push_str(" (multislot value (type FLOAT");
            if let Some(true) = nullable {
                def.push_str(" SYMBOL) (allowed-symbols nil");
            }
            def.push(')');
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
        Property::StringArray { nullable, default } => {
            def.push_str(" (multislot value (type STRING");
            if let Some(true) = nullable {
                def.push_str(" SYMBOL) (allowed-symbols nil");
            }
            def.push(')');
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
        Property::SymbolArray { nullable, default, allowed_values } => {
            def.push_str(" (multislot value (type SYMBOL)");
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
        Property::ObjectArray { nullable, default, .. } => {
            def.push_str(" (multislot value (type SYMBOL)");
            if let Some(def_val) = default {
                let def_str = def_val.iter().map(|o| o.as_str()).collect::<Vec<_>>().join(" ");
                def.push_str(&format!(" (default {})", def_str));
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

fn set_prop(env: &Environment, fb: FactBuilder, property: &Property, value: Value, time: Option<DateTime<Utc>>) -> Result<FactBuilder, KnowledgeBaseError> {
    let builder = match (property, value) {
        (Property::Bool { .. }, Value::Bool(b)) => fb.put_symbol("value", if b { "TRUE" } else { "FALSE" }).map_err(|e| KnowledgeBaseError::KBError(format!("Failed to set bool property value: {}", e))),
        (Property::Bool { nullable: Some(true), .. }, Value::Null) => fb.put_symbol("value", "nil").map_err(|e| KnowledgeBaseError::KBError(format!("Failed to set null value for bool property: {}", e))),
        (Property::Int { .. }, Value::Int(i)) => fb.put_int("value", i).map_err(|e| KnowledgeBaseError::KBError(format!("Failed to set int property value: {}", e))),
        (Property::Int { nullable: Some(true), .. }, Value::Null) => fb.put_symbol("value", "nil").map_err(|e| KnowledgeBaseError::KBError(format!("Failed to set null value for int property: {}", e))),
        (Property::Float { .. }, Value::Float(f)) => fb.put_float("value", f).map_err(|e| KnowledgeBaseError::KBError(format!("Failed to set float property value: {}", e))),
        (Property::Float { nullable: Some(true), .. }, Value::Null) => fb.put_symbol("value", "nil").map_err(|e| KnowledgeBaseError::KBError(format!("Failed to set null value for float property: {}", e))),
        (Property::String { .. }, Value::String(s)) => fb.put_string("value", s.as_str()).map_err(|e| KnowledgeBaseError::KBError(format!("Failed to set string property value: {}", e))),
        (Property::String { nullable: Some(true), .. }, Value::Null) => fb.put_symbol("value", "nil").map_err(|e| KnowledgeBaseError::KBError(format!("Failed to set null value for string property: {}", e))),
        (Property::Symbol { .. }, Value::Symbol(s)) => fb.put_symbol("value", s.as_str()).map_err(|e| KnowledgeBaseError::KBError(format!("Failed to set symbol property value: {}", e))),
        (Property::Symbol { nullable: Some(true), .. }, Value::Null) => fb.put_symbol("value", "nil").map_err(|e| KnowledgeBaseError::KBError(format!("Failed to set null value for symbol property: {}", e))),
        (Property::Object { .. }, Value::Object(o)) => fb.put_symbol("value", o.as_str()).map_err(|e| KnowledgeBaseError::KBError(format!("Failed to set object property value: {}", e))),
        (Property::Object { nullable: Some(true), .. }, Value::Null) => fb.put_symbol("value", "nil").map_err(|e| KnowledgeBaseError::KBError(format!("Failed to set null value for object property: {}", e))),
        (Property::BoolArray { .. }, Value::BoolArray(arr)) => {
            let builder = env.multifield_builder(arr.len()).map_err(|e| KnowledgeBaseError::KBError(format!("Failed to create multifield for bool array: {}", e)))?;
            let builder = arr.iter().fold(builder, |bld, &b| bld.put_symbol(if b { "TRUE" } else { "FALSE" }));
            fb.put_multifield("value", builder.create()).map_err(|e| KnowledgeBaseError::KBError(format!("Failed to set bool array property value: {}", e)))
        }
        (Property::BoolArray { nullable: Some(true), .. }, Value::Null) => fb.put_symbol("value", "nil").map_err(|e| KnowledgeBaseError::KBError(format!("Failed to set null value for bool array property: {}", e))),
        (Property::IntArray { .. }, Value::IntArray(arr)) => {
            let builder = env.multifield_builder(arr.len()).map_err(|e| KnowledgeBaseError::KBError(format!("Failed to create multifield for int array: {}", e)))?;
            let builder = arr.iter().fold(builder, |bld, &i| bld.put_int(i));
            fb.put_multifield("value", builder.create()).map_err(|e| KnowledgeBaseError::KBError(format!("Failed to set int array property value: {}", e)))
        }
        (Property::IntArray { nullable: Some(true), .. }, Value::Null) => fb.put_symbol("value", "nil").map_err(|e| KnowledgeBaseError::KBError(format!("Failed to set null value for int array property: {}", e))),
        (Property::FloatArray { .. }, Value::FloatArray(arr)) => {
            let builder = env.multifield_builder(arr.len()).map_err(|e| KnowledgeBaseError::KBError(format!("Failed to create multifield for float array: {}", e)))?;
            let builder = arr.iter().fold(builder, |bld, &f| bld.put_float(f));
            fb.put_multifield("value", builder.create()).map_err(|e| KnowledgeBaseError::KBError(format!("Failed to set float array property value: {}", e)))
        }
        (Property::FloatArray { nullable: Some(true), .. }, Value::Null) => fb.put_symbol("value", "nil").map_err(|e| KnowledgeBaseError::KBError(format!("Failed to set null value for float array property: {}", e))),
        (Property::StringArray { .. }, Value::StringArray(arr)) => {
            let builder = env.multifield_builder(arr.len()).map_err(|e| KnowledgeBaseError::KBError(format!("Failed to create multifield for string array: {}", e)))?;
            let builder = arr.iter().fold(builder, |bld, s| bld.put_string(s.as_str()));
            fb.put_multifield("value", builder.create()).map_err(|e| KnowledgeBaseError::KBError(format!("Failed to set string array property value: {}", e)))
        }
        (Property::StringArray { nullable: Some(true), .. }, Value::Null) => fb.put_symbol("value", "nil").map_err(|e| KnowledgeBaseError::KBError(format!("Failed to set null value for string array property: {}", e))),
        (Property::SymbolArray { .. }, Value::StringArray(arr)) => {
            let builder = env.multifield_builder(arr.len()).map_err(|e| KnowledgeBaseError::KBError(format!("Failed to create multifield for symbol array: {}", e)))?;
            let builder = arr.iter().fold(builder, |bld, s| bld.put_symbol(s.as_str()));
            fb.put_multifield("value", builder.create()).map_err(|e| KnowledgeBaseError::KBError(format!("Failed to set symbol array property value: {}", e)))
        }
        (Property::SymbolArray { nullable: Some(true), .. }, Value::Null) => fb.put_symbol("value", "nil").map_err(|e| KnowledgeBaseError::KBError(format!("Failed to set null value for symbol array property: {}", e))),
        (Property::ObjectArray { .. }, Value::StringArray(arr)) => {
            let builder = env.multifield_builder(arr.len()).map_err(|e| KnowledgeBaseError::KBError(format!("Failed to create multifield for object array: {}", e)))?;
            let builder = arr.iter().fold(builder, |bld, o| bld.put_symbol(o.as_str()));
            fb.put_multifield("value", builder.create()).map_err(|e| KnowledgeBaseError::KBError(format!("Failed to set object array property value: {}", e)))
        }
        (Property::ObjectArray { nullable: Some(true), .. }, Value::Null) => fb.put_symbol("value", "nil").map_err(|e| KnowledgeBaseError::KBError(format!("Failed to set null value for object array property: {}", e))),
        _ => Err(KnowledgeBaseError::KBError("Property type and value type do not match".to_string())),
    };
    if let Some(t) = time { builder.and_then(|fb| fb.put_int("time", t.timestamp()).map_err(|e| KnowledgeBaseError::KBError(format!("Failed to set time slot for property value: {}", e)))) } else { builder }
}

fn update_prop(env: &Environment, fm: FactModifier, property: &Property, value: Value, time: Option<DateTime<Utc>>) -> Result<FactModifier, KnowledgeBaseError> {
    let modifier = match (property, value) {
        (Property::Bool { .. }, Value::Bool(b)) => fm.put_symbol("value", if b { "TRUE" } else { "FALSE" }).map_err(|e| KnowledgeBaseError::KBError(format!("Failed to set bool property value: {}", e))),
        (Property::Bool { nullable: Some(true), .. }, Value::Null) => fm.put_symbol("value", "nil").map_err(|e| KnowledgeBaseError::KBError(format!("Failed to set null value for bool property: {}", e))),
        (Property::Int { .. }, Value::Int(i)) => fm.put_int("value", i).map_err(|e| KnowledgeBaseError::KBError(format!("Failed to set int property value: {}", e))),
        (Property::Int { nullable: Some(true), .. }, Value::Null) => fm.put_symbol("value", "nil").map_err(|e| KnowledgeBaseError::KBError(format!("Failed to set null value for int property: {}", e))),
        (Property::Float { .. }, Value::Float(f)) => fm.put_float("value", f).map_err(|e| KnowledgeBaseError::KBError(format!("Failed to set float property value: {}", e))),
        (Property::Float { nullable: Some(true), .. }, Value::Null) => fm.put_symbol("value", "nil").map_err(|e| KnowledgeBaseError::KBError(format!("Failed to set null value for float property: {}", e))),
        (Property::String { .. }, Value::String(s)) => fm.put_string("value", s.as_str()).map_err(|e| KnowledgeBaseError::KBError(format!("Failed to set string property value: {}", e))),
        (Property::String { nullable: Some(true), .. }, Value::Null) => fm.put_symbol("value", "nil").map_err(|e| KnowledgeBaseError::KBError(format!("Failed to set null value for string property: {}", e))),
        (Property::Symbol { .. }, Value::Symbol(s)) => fm.put_symbol("value", s.as_str()).map_err(|e| KnowledgeBaseError::KBError(format!("Failed to set symbol property value: {}", e))),
        (Property::Symbol { nullable: Some(true), .. }, Value::Null) => fm.put_symbol("value", "nil").map_err(|e| KnowledgeBaseError::KBError(format!("Failed to set null value for symbol property: {}", e))),
        (Property::Object { .. }, Value::Object(o)) => fm.put_symbol("value", o.as_str()).map_err(|e| KnowledgeBaseError::KBError(format!("Failed to set object property value: {}", e))),
        (Property::Object { nullable: Some(true), .. }, Value::Null) => fm.put_symbol("value", "nil").map_err(|e| KnowledgeBaseError::KBError(format!("Failed to set null value for object property: {}", e))),
        (Property::BoolArray { .. }, Value::BoolArray(arr)) => {
            let builder = env.multifield_builder(arr.len()).map_err(|e| KnowledgeBaseError::KBError(format!("Failed to create multifield for bool array: {}", e)))?;
            let builder = arr.iter().fold(builder, |bld, &b| bld.put_symbol(if b { "TRUE" } else { "FALSE" }));
            fm.put_multifield("value", builder.create()).map_err(|e| KnowledgeBaseError::KBError(format!("Failed to set bool array property value: {}", e)))
        }
        (Property::BoolArray { nullable: Some(true), .. }, Value::Null) => fm.put_symbol("value", "nil").map_err(|e| KnowledgeBaseError::KBError(format!("Failed to set null value for bool array property: {}", e))),
        (Property::IntArray { .. }, Value::IntArray(arr)) => {
            let builder = env.multifield_builder(arr.len()).map_err(|e| KnowledgeBaseError::KBError(format!("Failed to create multifield for int array: {}", e)))?;
            let builder = arr.iter().fold(builder, |bld, &i| bld.put_int(i));
            fm.put_multifield("value", builder.create()).map_err(|e| KnowledgeBaseError::KBError(format!("Failed to set int array property value: {}", e)))
        }
        (Property::IntArray { nullable: Some(true), .. }, Value::Null) => fm.put_symbol("value", "nil").map_err(|e| KnowledgeBaseError::KBError(format!("Failed to set null value for int array property: {}", e))),
        (Property::FloatArray { .. }, Value::FloatArray(arr)) => {
            let builder = env.multifield_builder(arr.len()).map_err(|e| KnowledgeBaseError::KBError(format!("Failed to create multifield for float array: {}", e)))?;
            let builder = arr.iter().fold(builder, |bld, &f| bld.put_float(f));
            fm.put_multifield("value", builder.create()).map_err(|e| KnowledgeBaseError::KBError(format!("Failed to set float array property value: {}", e)))
        }
        (Property::FloatArray { nullable: Some(true), .. }, Value::Null) => fm.put_symbol("value", "nil").map_err(|e| KnowledgeBaseError::KBError(format!("Failed to set null value for float array property: {}", e))),
        (Property::StringArray { .. }, Value::StringArray(arr)) => {
            let builder = env.multifield_builder(arr.len()).map_err(|e| KnowledgeBaseError::KBError(format!("Failed to create multifield for string array: {}", e)))?;
            let builder = arr.iter().fold(builder, |bld, s| bld.put_string(s.as_str()));
            fm.put_multifield("value", builder.create()).map_err(|e| KnowledgeBaseError::KBError(format!("Failed to set string array property value: {}", e)))
        }
        (Property::StringArray { nullable: Some(true), .. }, Value::Null) => fm.put_symbol("value", "nil").map_err(|e| KnowledgeBaseError::KBError(format!("Failed to set null value for string array property: {}", e))),
        (Property::SymbolArray { .. }, Value::StringArray(arr)) => {
            let builder = env.multifield_builder(arr.len()).map_err(|e| KnowledgeBaseError::KBError(format!("Failed to create multifield for symbol array: {}", e)))?;
            let builder = arr.iter().fold(builder, |bld, s| bld.put_symbol(s.as_str()));
            fm.put_multifield("value", builder.create()).map_err(|e| KnowledgeBaseError::KBError(format!("Failed to set symbol array property value: {}", e)))
        }
        (Property::SymbolArray { nullable: Some(true), .. }, Value::Null) => fm.put_symbol("value", "nil").map_err(|e| KnowledgeBaseError::KBError(format!("Failed to set null value for symbol array property: {}", e))),
        (Property::ObjectArray { .. }, Value::StringArray(arr)) => {
            let builder = env.multifield_builder(arr.len()).map_err(|e| KnowledgeBaseError::KBError(format!("Failed to create multifield for object array: {}", e)))?;
            let builder = arr.iter().fold(builder, |bld, o| bld.put_symbol(o.as_str()));
            fm.put_multifield("value", builder.create()).map_err(|e| KnowledgeBaseError::KBError(format!("Failed to set object array property value: {}", e)))
        }
        (Property::ObjectArray { nullable: Some(true), .. }, Value::Null) => fm.put_symbol("value", "nil").map_err(|e| KnowledgeBaseError::KBError(format!("Failed to set null value for object array property: {}", e))),
        _ => Err(KnowledgeBaseError::KBError("Property type and value type do not match".to_string())),
    };
    if let Some(t) = time { modifier.and_then(|fm| fm.put_int("time", t.timestamp()).map_err(|e| KnowledgeBaseError::KBError(format!("Failed to set time slot for property value: {}", e)))) } else { modifier }
}

fn get_default(property: &Property) -> Option<Value> {
    match property {
        Property::Bool { default, nullable, .. } => default.map(Value::Bool).or_else(|| if *nullable == Some(true) { Some(Value::Null) } else { None }),
        Property::Int { default, nullable, .. } => default.map(Value::Int).or_else(|| if *nullable == Some(true) { Some(Value::Null) } else { None }),
        Property::Float { default, nullable, .. } => default.map(Value::Float).or_else(|| if *nullable == Some(true) { Some(Value::Null) } else { None }),
        Property::String { default, nullable, .. } => default.clone().map(Value::String).or_else(|| if *nullable == Some(true) { Some(Value::Null) } else { None }),
        Property::Symbol { default, nullable, .. } => default.clone().map(Value::Symbol).or_else(|| if *nullable == Some(true) { Some(Value::Null) } else { None }),
        Property::Object { default, nullable, .. } => default.clone().map(Value::Object).or_else(|| if *nullable == Some(true) { Some(Value::Null) } else { None }),
        Property::BoolArray { default, nullable } => default.clone().map(Value::BoolArray).or_else(|| if *nullable == Some(true) { Some(Value::Null) } else { None }),
        Property::IntArray { default, nullable, .. } => default.clone().map(Value::IntArray).or_else(|| if *nullable == Some(true) { Some(Value::Null) } else { None }),
        Property::FloatArray { default, nullable, .. } => default.clone().map(Value::FloatArray).or_else(|| if *nullable == Some(true) { Some(Value::Null) } else { None }),
        Property::StringArray { default, nullable } => default.clone().map(Value::StringArray).or_else(|| if *nullable == Some(true) { Some(Value::Null) } else { None }),
        Property::SymbolArray { default, nullable, .. } => default.clone().map(Value::StringArray).or_else(|| if *nullable == Some(true) { Some(Value::Null) } else { None }),
        Property::ObjectArray { default, nullable, .. } => default.clone().map(Value::StringArray).or_else(|| if *nullable == Some(true) { Some(Value::Null) } else { None }),
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use std::collections::{HashMap, HashSet};

    // Initialization Tests
    #[test]
    fn test_new_knowledge_base() {
        let kb = CLIPSKnowledgeBase::new();
        assert_eq!(kb.get_classes().len(), 0);
        assert_eq!(kb.get_objects().len(), 0);
        assert_eq!(kb.get_rules().len(), 0);
    }

    #[test]
    fn test_default_knowledge_base() {
        let kb = CLIPSKnowledgeBase::default();
        assert_eq!(kb.get_classes().len(), 0);
        assert_eq!(kb.get_objects().len(), 0);
        assert_eq!(kb.get_rules().len(), 0);
    }

    // Class Creation Tests
    #[test]
    fn test_create_simple_class() {
        let mut kb = CLIPSKnowledgeBase::new();
        let class = Class {
            name: "TestClass".to_string(),
            parents: None,
            static_properties: None,
            dynamic_properties: None,
        };

        let result = kb.create_class(class);
        assert!(result.is_ok());
        assert_eq!(kb.get_classes().len(), 1);
    }

    #[test]
    fn test_get_class_by_name() {
        let mut kb = CLIPSKnowledgeBase::new();
        let class = Class {
            name: "TestClass".to_string(),
            parents: None,
            static_properties: None,
            dynamic_properties: None,
        };

        kb.create_class(class).unwrap();
        let retrieved = kb.get_class("TestClass");
        assert!(retrieved.is_some());
        assert_eq!(retrieved.unwrap().name, "TestClass");
    }

    #[test]
    fn test_create_class_with_static_bool_property() {
        let mut kb = CLIPSKnowledgeBase::new();
        let mut static_props = HashMap::new();
        static_props.insert("active".to_string(), Property::Bool { nullable: Some(false), default: Some(true) });

        let class = Class {
            name: "Device".to_string(),
            parents: None,
            static_properties: Some(static_props),
            dynamic_properties: None,
        };

        let result = kb.create_class(class);
        assert!(result.is_ok());
    }

    #[test]
    fn test_create_class_with_static_int_property() {
        let mut kb = CLIPSKnowledgeBase::new();
        let mut static_props = HashMap::new();
        static_props.insert("temperature".to_string(), Property::Int { nullable: Some(false), default: Some(25), min: Some(0), max: Some(100) });

        let class = Class {
            name: "Sensor".to_string(),
            parents: None,
            static_properties: Some(static_props),
            dynamic_properties: None,
        };

        let result = kb.create_class(class);
        assert!(result.is_ok());
    }

    #[test]
    fn test_create_class_with_static_float_property() {
        let mut kb = CLIPSKnowledgeBase::new();
        let mut static_props = HashMap::new();
        static_props.insert("voltage".to_string(), Property::Float { nullable: Some(false), default: Some(5.0), min: Some(0.0), max: Some(10.0) });

        let class = Class {
            name: "PowerSupply".to_string(),
            parents: None,
            static_properties: Some(static_props),
            dynamic_properties: None,
        };

        let result = kb.create_class(class);
        assert!(result.is_ok());
    }

    #[test]
    fn test_create_class_with_static_string_property() {
        let mut kb = CLIPSKnowledgeBase::new();
        let mut static_props = HashMap::new();
        static_props.insert("name".to_string(), Property::String { nullable: Some(false), default: Some("Default Name".to_string()) });

        let class = Class {
            name: "Entity".to_string(),
            parents: None,
            static_properties: Some(static_props),
            dynamic_properties: None,
        };

        let result = kb.create_class(class);
        assert!(result.is_ok());
    }

    #[test]
    fn test_create_class_with_static_symbol_property() {
        let mut kb = CLIPSKnowledgeBase::new();
        let mut static_props = HashMap::new();
        let mut allowed_symbols = HashSet::new();
        allowed_symbols.insert("STATE_ON".to_string());
        allowed_symbols.insert("STATE_OFF".to_string());

        static_props.insert(
            "state".to_string(),
            Property::Symbol {
                nullable: Some(false),
                default: Some("STATE_OFF".to_string()),
                allowed_values: Some(allowed_symbols),
            },
        );

        let class = Class {
            name: "Switch".to_string(),
            parents: None,
            static_properties: Some(static_props),
            dynamic_properties: None,
        };

        let result = kb.create_class(class);
        assert!(result.is_ok());
    }

    #[test]
    fn test_create_class_with_static_object_property() {
        let mut kb = CLIPSKnowledgeBase::new();
        let mut static_props = HashMap::new();
        static_props.insert("owner".to_string(), Property::Object { nullable: Some(true), default: None, class: "Person".to_string() });

        let class = Class {
            name: "Item".to_string(),
            parents: None,
            static_properties: Some(static_props),
            dynamic_properties: None,
        };

        let result = kb.create_class(class);
        assert!(result.is_ok());
    }

    #[test]
    fn test_create_class_with_array_properties() {
        let mut kb = CLIPSKnowledgeBase::new();
        let mut static_props = HashMap::new();

        static_props.insert("bool_array".to_string(), Property::BoolArray { nullable: Some(false), default: Some(vec![true, false, true]) });

        static_props.insert("int_array".to_string(), Property::IntArray { nullable: Some(false), default: Some(vec![1, 2, 3]), min: Some(0), max: Some(100) });

        static_props.insert("float_array".to_string(), Property::FloatArray { nullable: Some(false), default: Some(vec![1.5, 2.5, 3.5]), min: Some(0.0), max: Some(10.0) });

        static_props.insert("string_array".to_string(), Property::StringArray { nullable: Some(false), default: Some(vec!["a".to_string(), "b".to_string()]) });

        let class = Class {
            name: "ArrayHolder".to_string(),
            parents: None,
            static_properties: Some(static_props),
            dynamic_properties: None,
        };

        let result = kb.create_class(class);
        assert!(result.is_ok());
    }

    #[test]
    fn test_create_class_with_dynamic_properties() {
        let mut kb = CLIPSKnowledgeBase::new();
        let mut dynamic_props = HashMap::new();
        dynamic_props.insert("pressure".to_string(), Property::Float { nullable: Some(false), default: Some(0.0), min: None, max: None });

        let class = Class {
            name: "DynamicSensor".to_string(),
            parents: None,
            static_properties: None,
            dynamic_properties: Some(dynamic_props),
        };

        let result = kb.create_class(class);
        assert!(result.is_ok());
    }

    #[test]
    fn test_create_duplicate_class_fails() {
        let mut kb = CLIPSKnowledgeBase::new();
        let class = Class {
            name: "DuplicateClass".to_string(),
            parents: None,
            static_properties: None,
            dynamic_properties: None,
        };

        kb.create_class(class.clone()).unwrap();
        let result = kb.create_class(class);
        assert!(result.is_err());
        match result {
            Err(KnowledgeBaseError::ClassAlreadyExists(_)) => (),
            _ => panic!("Expected ClassAlreadyExists error"),
        }
    }

    // Object Creation Tests
    #[test]
    fn test_create_object_without_id_fails() {
        let mut kb = CLIPSKnowledgeBase::new();
        let mut classes = HashSet::new();
        classes.insert("TestClass".to_string());

        let object = Object { id: None, classes, properties: None, values: None };

        let result = kb.create_object(object);
        assert!(result.is_err());
    }

    #[test]
    fn test_create_object_with_nonexistent_class_fails() {
        let mut kb = CLIPSKnowledgeBase::new();
        let mut classes = HashSet::new();
        classes.insert("NonExistentClass".to_string());

        let object = Object { id: Some("obj1".to_string()), classes, properties: None, values: None };

        let result = kb.create_object(object);
        assert!(result.is_err());
    }

    #[test]
    fn test_create_simple_object() {
        let mut kb = CLIPSKnowledgeBase::new();
        let class = Class {
            name: "TestClass".to_string(),
            parents: None,
            static_properties: None,
            dynamic_properties: None,
        };
        kb.create_class(class).unwrap();

        let mut classes = HashSet::new();
        classes.insert("TestClass".to_string());

        let object = Object { id: Some("obj1".to_string()), classes, properties: None, values: None };

        let result = kb.create_object(object);
        assert!(result.is_ok());
        assert_eq!(kb.get_objects().len(), 1);
    }

    #[test]
    fn test_get_object_by_id() {
        let mut kb = CLIPSKnowledgeBase::new();
        let class = Class {
            name: "TestClass".to_string(),
            parents: None,
            static_properties: None,
            dynamic_properties: None,
        };
        kb.create_class(class).unwrap();

        let mut classes = HashSet::new();
        classes.insert("TestClass".to_string());

        let object = Object { id: Some("obj1".to_string()), classes, properties: None, values: None };
        kb.create_object(object).unwrap();

        let retrieved = kb.get_object("obj1");
        assert!(retrieved.is_some());
        assert_eq!(retrieved.unwrap().id, Some("obj1".to_string()));
    }

    #[test]
    fn test_create_duplicate_object_fails() {
        let mut kb = CLIPSKnowledgeBase::new();
        let class = Class {
            name: "TestClass".to_string(),
            parents: None,
            static_properties: None,
            dynamic_properties: None,
        };
        kb.create_class(class).unwrap();

        let mut classes = HashSet::new();
        classes.insert("TestClass".to_string());

        let object = Object { id: Some("obj1".to_string()), classes: classes.clone(), properties: None, values: None };

        kb.create_object(object.clone()).unwrap();
        let result = kb.create_object(object);
        assert!(result.is_err());
    }

    #[test]
    fn test_create_object_with_static_properties() {
        let mut kb = CLIPSKnowledgeBase::new();
        let mut static_props = HashMap::new();
        static_props.insert("count".to_string(), Property::Int { nullable: Some(false), default: Some(0), min: None, max: None });

        let class = Class {
            name: "Counter".to_string(),
            parents: None,
            static_properties: Some(static_props),
            dynamic_properties: None,
        };
        kb.create_class(class).unwrap();

        let mut classes = HashSet::new();
        classes.insert("Counter".to_string());

        let mut properties = HashMap::new();
        properties.insert("count".to_string(), Value::Int(42));

        let object = Object { id: Some("counter1".to_string()), classes, properties: Some(properties), values: None };

        let result = kb.create_object(object);
        assert!(result.is_ok());
    }

    #[test]
    fn test_create_object_with_dynamic_values() {
        let mut kb = CLIPSKnowledgeBase::new();
        let mut dynamic_props = HashMap::new();
        dynamic_props.insert("temperature".to_string(), Property::Float { nullable: Some(false), default: Some(20.0), min: None, max: None });

        let class = Class {
            name: "ThermometerDynamic".to_string(),
            parents: None,
            static_properties: None,
            dynamic_properties: Some(dynamic_props),
        };
        kb.create_class(class).unwrap();

        let mut classes = HashSet::new();
        classes.insert("ThermometerDynamic".to_string());

        let mut values = HashMap::new();
        values.insert("temperature".to_string(), (Value::Float(25.5), Utc::now()));

        let object = Object { id: Some("temp_sensor1".to_string()), classes, properties: None, values: Some(values) };

        let result = kb.create_object(object);
        assert!(result.is_ok());
    }

    // Adding Class Tests
    #[test]
    fn test_add_class_to_nonexistent_object_fails() {
        let mut kb = CLIPSKnowledgeBase::new();
        let class = Class {
            name: "TestClass".to_string(),
            parents: None,
            static_properties: None,
            dynamic_properties: None,
        };
        kb.create_class(class).unwrap();

        let result = kb.add_class("nonexistent", "TestClass");
        assert!(result.is_err());
    }

    #[test]
    fn test_add_nonexistent_class_to_object_fails() {
        let mut kb = CLIPSKnowledgeBase::new();
        let class = Class {
            name: "TestClass".to_string(),
            parents: None,
            static_properties: None,
            dynamic_properties: None,
        };
        kb.create_class(class).unwrap();

        let mut classes = HashSet::new();
        classes.insert("TestClass".to_string());

        let object = Object { id: Some("obj1".to_string()), classes, properties: None, values: None };
        kb.create_object(object).unwrap();

        let result = kb.add_class("obj1", &"NonExistentClass");
        assert!(result.is_err());
    }

    #[test]
    fn test_add_class_to_object() {
        let mut kb = CLIPSKnowledgeBase::new();

        let class1 = Class {
            name: "Class1".to_string(),
            parents: None,
            static_properties: None,
            dynamic_properties: None,
        };
        let class2 = Class {
            name: "Class2".to_string(),
            parents: None,
            static_properties: None,
            dynamic_properties: None,
        };

        kb.create_class(class1).unwrap();
        kb.create_class(class2).unwrap();

        let mut classes = HashSet::new();
        classes.insert("Class1".to_string());

        let object = Object { id: Some("obj1".to_string()), classes, properties: None, values: None };
        kb.create_object(object).unwrap();

        let result = kb.add_class("obj1", "Class2");
        assert!(result.is_ok());
    }

    // Setting Properties Tests
    #[test]
    fn test_set_properties_on_nonexistent_object_fails() {
        let mut kb = CLIPSKnowledgeBase::new();

        let mut properties = HashMap::new();
        properties.insert("prop".to_string(), Value::Int(42));

        let result = kb.set_properties("nonexistent", properties);
        assert!(result.is_err());
    }

    #[test]
    fn test_set_properties_on_object() {
        let mut kb = CLIPSKnowledgeBase::new();
        let mut static_props = HashMap::new();
        static_props.insert("value".to_string(), Property::Int { nullable: Some(false), default: Some(0), min: None, max: None });

        let class = Class {
            name: "Configurable".to_string(),
            parents: None,
            static_properties: Some(static_props),
            dynamic_properties: None,
        };
        kb.create_class(class).unwrap();

        let mut classes = HashSet::new();
        classes.insert("Configurable".to_string());

        let object = Object { id: Some("config1".to_string()), classes: classes.clone(), properties: None, values: None };
        kb.create_object(object).unwrap();

        let mut properties = HashMap::new();
        properties.insert("value".to_string(), Value::Int(100));

        let result = kb.set_properties("config1", properties);
        assert!(result.is_ok());
    }

    // Adding Values Tests
    #[test]
    fn test_add_values_on_nonexistent_object_fails() {
        let mut kb = CLIPSKnowledgeBase::new();
        let mut classes = HashSet::new();
        classes.insert("NonExistent".to_string());

        let mut values = HashMap::new();
        values.insert("value".to_string(), Value::Float(1.0));

        let result = kb.add_values("nonexistent", values, Utc::now());
        assert!(result.is_err());
    }

    #[test]
    fn test_add_values_to_object() {
        let mut kb = CLIPSKnowledgeBase::new();
        let mut dynamic_props = HashMap::new();
        dynamic_props.insert("measurement".to_string(), Property::Float { nullable: Some(false), default: Some(0.0), min: None, max: None });

        let class = Class {
            name: "TimeSeries".to_string(),
            parents: None,
            static_properties: None,
            dynamic_properties: Some(dynamic_props),
        };
        kb.create_class(class).unwrap();

        let mut classes = HashSet::new();
        classes.insert("TimeSeries".to_string());

        let object = Object { id: Some("ts1".to_string()), classes: classes.clone(), properties: None, values: None };
        kb.create_object(object).unwrap();

        let mut values = HashMap::new();
        values.insert("measurement".to_string(), Value::Float(42.5));

        let result = kb.add_values("ts1", values, Utc::now());
        assert!(result.is_ok());
    }

    // Rule Tests
    #[test]
    fn test_create_simple_rule() {
        let mut kb = CLIPSKnowledgeBase::new();
        let rule = Rule {
            name: "test_rule".to_string(),
            content: "(defrule test_rule (fact) => (assert (derived)))".to_string(),
        };

        let result = kb.create_rule(rule);
        assert!(result.is_ok());
        assert_eq!(kb.get_rules().len(), 1);
    }

    #[test]
    fn test_get_rule_by_name() {
        let mut kb = CLIPSKnowledgeBase::new();
        let rule = Rule {
            name: "my_rule".to_string(),
            content: "(defrule my_rule (fact) => (assert (result)))".to_string(),
        };

        kb.create_rule(rule).unwrap();
        let retrieved = kb.get_rule("my_rule");
        assert!(retrieved.is_some());
        assert_eq!(retrieved.unwrap().name, "my_rule");
    }

    #[test]
    fn test_create_duplicate_rule_fails() {
        let mut kb = CLIPSKnowledgeBase::new();
        let rule = Rule {
            name: "duplicate_rule".to_string(),
            content: "(defrule duplicate_rule (fact) => (assert (result)))".to_string(),
        };

        kb.create_rule(rule.clone()).unwrap();
        let result = kb.create_rule(rule);
        assert!(result.is_err());
    }

    // Helper Function Tests
    #[test]
    fn test_get_default_bool_with_default() {
        let property = Property::Bool { nullable: Some(false), default: Some(true) };
        let default_val = get_default(&property);
        assert_eq!(default_val, Some(Value::Bool(true)));
    }

    #[test]
    fn test_get_default_bool_nullable_without_default() {
        let property = Property::Bool { nullable: Some(true), default: None };
        let default_val = get_default(&property);
        assert_eq!(default_val, Some(Value::Null));
    }

    #[test]
    fn test_get_default_int_with_default() {
        let property = Property::Int { nullable: Some(false), default: Some(42), min: None, max: None };
        let default_val = get_default(&property);
        assert_eq!(default_val, Some(Value::Int(42)));
    }

    #[test]
    fn test_get_default_float_with_default() {
        let property = Property::Float { nullable: Some(false), default: Some(3.14), min: None, max: None };
        let default_val = get_default(&property);
        assert_eq!(default_val, Some(Value::Float(3.14)));
    }

    #[test]
    fn test_get_default_string_with_default() {
        let property = Property::String { nullable: Some(false), default: Some("hello".to_string()) };
        let default_val = get_default(&property);
        assert_eq!(default_val, Some(Value::String("hello".to_string())));
    }

    #[test]
    fn test_get_default_symbol_with_default() {
        let property = Property::Symbol { nullable: Some(false), default: Some("SYMBOL_VALUE".to_string()), allowed_values: None };
        let default_val = get_default(&property);
        assert_eq!(default_val, Some(Value::Symbol("SYMBOL_VALUE".to_string())));
    }

    #[test]
    fn test_get_default_object_with_default() {
        let property = Property::Object { nullable: Some(false), default: Some("obj_id".to_string()), class: "MyClass".to_string() };
        let default_val = get_default(&property);
        assert_eq!(default_val, Some(Value::Object("obj_id".to_string())));
    }

    #[test]
    fn test_get_default_bool_array_with_default() {
        let property = Property::BoolArray { nullable: Some(false), default: Some(vec![true, false]) };
        let default_val = get_default(&property);
        assert_eq!(default_val, Some(Value::BoolArray(vec![true, false])));
    }

    #[test]
    fn test_get_default_int_array_with_default() {
        let property = Property::IntArray { nullable: Some(false), default: Some(vec![1, 2, 3]), min: None, max: None };
        let default_val = get_default(&property);
        assert_eq!(default_val, Some(Value::IntArray(vec![1, 2, 3])));
    }

    #[test]
    fn test_get_default_float_array_with_default() {
        let property = Property::FloatArray { nullable: Some(false), default: Some(vec![1.5, 2.5]), min: None, max: None };
        let default_val = get_default(&property);
        assert_eq!(default_val, Some(Value::FloatArray(vec![1.5, 2.5])));
    }

    #[test]
    fn test_get_default_string_array_with_default() {
        let property = Property::StringArray { nullable: Some(false), default: Some(vec!["a".to_string(), "b".to_string()]) };
        let default_val = get_default(&property);
        assert_eq!(default_val, Some(Value::StringArray(vec!["a".to_string(), "b".to_string()])));
    }

    #[test]
    fn test_get_default_symbol_array_with_default() {
        let property = Property::SymbolArray {
            nullable: Some(false),
            default: Some(vec!["SYM1".to_string(), "SYM2".to_string()]),
            allowed_values: None,
        };
        let default_val = get_default(&property);
        // Note: SymbolArray returns StringArray due to the implementation
        assert_eq!(default_val, Some(Value::StringArray(vec!["SYM1".to_string(), "SYM2".to_string()])));
    }

    #[test]
    fn test_prop_deftemplate_bool_static() {
        let class = Class {
            name: "TestClass".to_string(),
            parents: None,
            static_properties: None,
            dynamic_properties: None,
        };
        let property = Property::Bool { nullable: Some(true), default: Some(false) };

        let template = prop_deftemplate(&class, "active", &property, true);
        assert!(template.contains("TestClass_active"));
        assert!(template.contains("TRUE"));
        assert!(template.contains("FALSE"));
        assert!(template.contains("nil"));
    }

    #[test]
    fn test_prop_deftemplate_int_with_range() {
        let class = Class {
            name: "TestClass".to_string(),
            parents: None,
            static_properties: None,
            dynamic_properties: None,
        };
        let property = Property::Int { nullable: Some(false), default: Some(50), min: Some(0), max: Some(100) };

        let template = prop_deftemplate(&class, "percentage", &property, true);
        assert!(template.contains("TestClass_percentage"));
        assert!(template.contains("range"));
        assert!(template.contains("0"));
        assert!(template.contains("100"));
    }

    #[test]
    fn test_prop_deftemplate_float_dynamic() {
        let class = Class {
            name: "TestClass".to_string(),
            parents: None,
            static_properties: None,
            dynamic_properties: None,
        };
        let property = Property::Float { nullable: Some(false), default: Some(1.5), min: None, max: None };

        let template = prop_deftemplate(&class, "metric", &property, false);
        assert!(template.contains("TestClass_metric"));
        assert!(template.contains("time"));
        assert!(template.contains("1.5"));
    }

    #[test]
    fn test_prop_deftemplate_symbol_with_allowed_values() {
        let class = Class {
            name: "TestClass".to_string(),
            parents: None,
            static_properties: None,
            dynamic_properties: None,
        };
        let mut allowed = HashSet::new();
        allowed.insert("ON".to_string());
        allowed.insert("OFF".to_string());

        let property = Property::Symbol { nullable: Some(false), default: Some("OFF".to_string()), allowed_values: Some(allowed) };

        let template = prop_deftemplate(&class, "state", &property, true);
        assert!(template.contains("TestClass_state"));
        assert!(template.contains("ON"));
        assert!(template.contains("OFF"));
    }

    #[test]
    fn test_prop_deftemplate_int_array_with_range() {
        let class = Class {
            name: "TestClass".to_string(),
            parents: None,
            static_properties: None,
            dynamic_properties: None,
        };
        let property = Property::IntArray { nullable: Some(false), default: Some(vec![1, 2, 3]), min: Some(0), max: Some(10) };

        let template = prop_deftemplate(&class, "values", &property, true);
        assert!(template.contains("TestClass_values"));
        assert!(template.contains("multislot"));
        assert!(template.contains("range"));
    }

    #[test]
    fn test_prop_deftemplate_string_array() {
        let class = Class {
            name: "TestClass".to_string(),
            parents: None,
            static_properties: None,
            dynamic_properties: None,
        };
        let property = Property::StringArray { nullable: Some(true), default: Some(vec!["item1".to_string(), "item2".to_string()]) };

        let template = prop_deftemplate(&class, "items", &property, true);
        assert!(template.contains("TestClass_items"));
        assert!(template.contains("multislot"));
        assert!(template.contains("STRING"));
    }

    #[test]
    fn test_complex_workflow() {
        // Create a knowledge base
        let mut kb = CLIPSKnowledgeBase::new();

        // Create a class with mixed properties
        let mut static_props = HashMap::new();
        static_props.insert("name".to_string(), Property::String { nullable: Some(false), default: Some("Unknown".to_string()) });
        static_props.insert("enabled".to_string(), Property::Bool { nullable: Some(false), default: Some(true) });

        let mut dynamic_props = HashMap::new();
        dynamic_props.insert("reading".to_string(), Property::Float { nullable: Some(false), default: Some(0.0), min: None, max: None });

        let class = Class {
            name: "Sensor".to_string(),
            parents: None,
            static_properties: Some(static_props),
            dynamic_properties: Some(dynamic_props),
        };

        // Create the class
        assert!(kb.create_class(class).is_ok());

        // Create an object
        let mut classes = HashSet::new();
        classes.insert("Sensor".to_string());

        let mut properties = HashMap::new();
        properties.insert("name".to_string(), Value::String("Temperature Sensor".to_string()));

        let object = Object {
            id: Some("sensor1".to_string()),
            classes: classes.clone(),
            properties: Some(properties),
            values: None,
        };

        assert!(kb.create_object(object).is_ok());

        // Add a value
        let mut values = HashMap::new();
        values.insert("reading".to_string(), Value::Float(23.5));

        assert!(kb.add_values("sensor1", values, Utc::now()).is_ok());

        // Verify the setup
        assert_eq!(kb.get_classes().len(), 1);
        assert_eq!(kb.get_objects().len(), 1);
        assert_eq!(kb.get_class("Sensor").is_some(), true);
        assert_eq!(kb.get_object("sensor1").is_some(), true);
    }

    #[test]
    fn test_multiple_classes_and_objects() {
        let mut kb = CLIPSKnowledgeBase::new();

        // Create multiple classes
        for i in 0..5 {
            let class = Class {
                name: format!("Class{}", i),
                parents: None,
                static_properties: None,
                dynamic_properties: None,
            };
            assert!(kb.create_class(class).is_ok());
        }

        // Create multiple objects for each class
        for i in 0..5 {
            let mut classes = HashSet::new();
            classes.insert(format!("Class{}", i));

            let object = Object { id: Some(format!("obj{}", i)), classes, properties: None, values: None };

            assert!(kb.create_object(object).is_ok());
        }

        assert_eq!(kb.get_classes().len(), 5);
        assert_eq!(kb.get_objects().len(), 5);
    }

    #[test]
    fn test_object_with_multiple_classes() {
        let mut kb = CLIPSKnowledgeBase::new();

        // Create two classes
        let class1 = Class {
            name: "Class1".to_string(),
            parents: None,
            static_properties: None,
            dynamic_properties: None,
        };
        let class2 = Class {
            name: "Class2".to_string(),
            parents: None,
            static_properties: None,
            dynamic_properties: None,
        };

        kb.create_class(class1).unwrap();
        kb.create_class(class2).unwrap();

        // Create an object with both classes
        let mut classes = HashSet::new();
        classes.insert("Class1".to_string());
        classes.insert("Class2".to_string());

        let object = Object { id: Some("multi_obj".to_string()), classes, properties: None, values: None };

        assert!(kb.create_object(object).is_ok());
        let retrieved = kb.get_object("multi_obj").unwrap();
        assert_eq!(retrieved.classes.len(), 2);
    }

    #[test]
    fn test_nullable_properties() {
        let mut kb = CLIPSKnowledgeBase::new();

        let mut static_props = HashMap::new();
        static_props.insert("optional_field".to_string(), Property::String { nullable: Some(true), default: None });

        let class = Class {
            name: "NullableClass".to_string(),
            parents: None,
            static_properties: Some(static_props),
            dynamic_properties: None,
        };

        assert!(kb.create_class(class).is_ok());

        let mut classes = HashSet::new();
        classes.insert("NullableClass".to_string());

        let object = Object { id: Some("nullable_obj".to_string()), classes, properties: None, values: None };

        assert!(kb.create_object(object).is_ok());
    }

    #[test]
    fn test_run_execution() {
        let mut kb = CLIPSKnowledgeBase::new();
        // Just test that run doesn't panic
        assert!(kb.run().is_ok());
    }

    #[test]
    fn test_dynamic_property_with_rule_threshold() {
        // Create a knowledge base
        let mut kb = CLIPSKnowledgeBase::new();

        // Create a class with a dynamic property (temperature monitoring)
        let mut dynamic_props = HashMap::new();
        dynamic_props.insert("temperature".to_string(), Property::Float { nullable: Some(false), default: Some(20.0), min: None, max: None });

        let class = Class {
            name: "ThermometerMonitor".to_string(),
            parents: None,
            static_properties: None,
            dynamic_properties: Some(dynamic_props),
        };

        // Create the class
        assert!(kb.create_class(class).is_ok());

        // Create an object for temperature monitoring
        let mut classes = HashSet::new();
        classes.insert("ThermometerMonitor".to_string());

        let object = Object { id: Some("thermo1".to_string()), classes, properties: None, values: None };

        assert!(kb.create_object(object).is_ok());

        // Register a rule that triggers when temperature exceeds threshold (30 degrees)
        // When triggered, it will call the add-data UDF with existing dynamic properties
        // add-data signature: (add-data object-id param-names-multifield param-values-multifield [timestamp])
        let rule = Rule {
            name: "temperature_alert_rule".to_string(),
            content: "(defrule temperature_alert_rule
                (ThermometerMonitor_temperature (id ?id) (value ?temp&:(> ?temp 35)))
                =>
                (add-data ?id (create$ temperature) (create$ 99.9))
            )"
            .to_string(),
        };

        assert!(kb.create_rule(rule).is_ok());

        // Initial values - set temperature below threshold (should not trigger rule)
        let mut values = HashMap::new();
        values.insert("temperature".to_string(), Value::Float(25.0));
        assert!(kb.add_values("thermo1", values, Utc::now()).is_ok());

        // Run the knowledge base with low temperature
        assert!(kb.run().is_ok());

        // Now update temperature to exceed threshold (should trigger rule)
        let mut values_high = HashMap::new();
        values_high.insert("temperature".to_string(), Value::Float(35.0));
        assert!(kb.add_values("thermo1", values_high, Utc::now()).is_ok());

        // Run the knowledge base again - this should trigger the rule and call add-data UDF
        assert!(kb.run().is_ok());

        // Verify the object still exists after rule execution
        assert_eq!(kb.get_object("thermo1").is_some(), true);

        // Verify the rule exists
        assert_eq!(kb.get_rule("temperature_alert_rule").is_some(), true);
    }

    #[test]
    fn test_multiple_dynamic_properties_with_multiple_rules() {
        // Create a knowledge base
        let mut kb = CLIPSKnowledgeBase::new();

        // Create a complex sensor class with multiple dynamic properties
        let mut dynamic_props = HashMap::new();
        dynamic_props.insert("temperature".to_string(), Property::Float { nullable: Some(false), default: Some(20.0), min: None, max: None });
        dynamic_props.insert("humidity".to_string(), Property::Float { nullable: Some(false), default: Some(50.0), min: None, max: None });
        dynamic_props.insert("pressure".to_string(), Property::Float { nullable: Some(false), default: Some(1013.0), min: None, max: None });

        let class = Class {
            name: "EnvironmentalSensor".to_string(),
            parents: None,
            static_properties: None,
            dynamic_properties: Some(dynamic_props),
        };

        assert!(kb.create_class(class).is_ok());

        // Create an object
        let mut classes = HashSet::new();
        classes.insert("EnvironmentalSensor".to_string());

        let object = Object { id: Some("env_sensor1".to_string()), classes, properties: None, values: None };

        assert!(kb.create_object(object).is_ok());

        // Register rule for temperature threshold - calls add-data UDF to update the temperature parameter
        // add-data takes: object_id, param-names (multifield), param-values (multifield)
        let temp_rule = Rule {
            name: "high_temperature_alert".to_string(),
            content: "(defrule high_temperature_alert
                (EnvironmentalSensor_temperature (id ?id) (value ?temp&:(> ?temp 35)))
                =>
                (add-data ?id (create$ temperature) (create$ ?temp))
            )"
            .to_string(),
        };

        assert!(kb.create_rule(temp_rule).is_ok());

        // Register rule for humidity threshold - uses add-data UDF to update humidity parameter
        let humidity_rule = Rule {
            name: "high_humidity_alert".to_string(),
            content: "(defrule high_humidity_alert
                (EnvironmentalSensor_humidity (id ?id) (value ?humid&:(> ?humid 80)))
                =>
                (add-data ?id (create$ humidity) (create$ ?humid))
            )"
            .to_string(),
        };

        assert!(kb.create_rule(humidity_rule).is_ok());

        // Update all properties with values that trigger both rules
        let mut values = HashMap::new();
        values.insert("temperature".to_string(), Value::Float(40.0)); // Exceeds 35 threshold
        values.insert("humidity".to_string(), Value::Float(85.0)); // Exceeds 80 threshold
        values.insert("pressure".to_string(), Value::Float(1020.0)); // Normal

        assert!(kb.add_values("env_sensor1", values, Utc::now()).is_ok());

        // Run the knowledge base - both rules should trigger
        assert!(kb.run().is_ok());

        // Verify both rules exist
        assert_eq!(kb.get_rule("high_temperature_alert").is_some(), true);
        assert_eq!(kb.get_rule("high_humidity_alert").is_some(), true);
    }
}

use crate::{
    kb::{KnowledgeBaseError, KnowledgeBaseEvent},
    model::{Class, Object, Property, Rule, TimedValue, Value},
};
use chrono::{DateTime, Utc};
use clips::{ClipsValue, Environment, Fact, FactBuilder, FactModifier, Type};
use std::{cell::RefCell, collections::HashMap, rc::Rc};
use tracing::debug;

pub type Callback = Box<dyn Fn(KnowledgeBaseEvent)>;

pub struct CLIPSKnowledgeBase {
    classes: HashMap<String, Class>,
    objects: HashMap<String, Object>,
    rules: HashMap<String, Rule>,
    instances: HashMap<String, HashMap<String, Fact>>,               // class name -> object id -> fact
    values: HashMap<String, HashMap<String, HashMap<String, Fact>>>, // class name -> object id -> property name -> fact
    env: Environment,
    callback: Rc<RefCell<Option<Callback>>>,
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

        let add_data_callback = callback.clone();
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
                if let Some(cb) = add_data_callback.borrow().as_ref() {
                    cb(KnowledgeBaseEvent::AddedValues(object_id.clone(), values.clone(), date_time));
                }
                ClipsValue::Void()
            })
            .map_err(|e| KnowledgeBaseError::KBError(format!("Failed to add UDF to CLIPS environment: {}", e)))?;

        let add_class_callback = callback.clone();
        kb.env
            .add_udf("add-class", None, 2, 2, vec![Type(Type::SYMBOL), Type(Type::SYMBOL)], move |_env, ctx| {
                let object_id = ctx.get_next_argument(Type(Type::SYMBOL)).expect("Failed to get object ID argument for add-class UDF");
                let object_id = if let ClipsValue::Symbol(s) = object_id { s } else { panic!("Expected symbol for object ID argument in add-class UDF") };
                let class_name = ctx.get_next_argument(Type(Type::SYMBOL)).expect("Failed to get class name argument for add-class UDF");
                let class_name = if let ClipsValue::Symbol(s) = class_name { s } else { panic!("Expected symbol for class name argument in add-class UDF") };
                if let Some(cb) = add_class_callback.borrow().as_ref() {
                    cb(KnowledgeBaseEvent::AddedClass(object_id.clone(), class_name.clone()));
                }
                ClipsValue::Void()
            })
            .map_err(|e| KnowledgeBaseError::KBError(format!("Failed to add UDF to CLIPS environment: {}", e)))?;
        Ok(kb)
    }
}

impl crate::kb::KnowledgeBase for CLIPSKnowledgeBase {
    fn get_classes(&self) -> Vec<&Class> {
        self.classes.values().collect()
    }

    fn get_class(&self, name: &str) -> Option<&Class> {
        self.classes.get(name)
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
                        } else {
                            let def = get_default(prop);
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
                            let fb = set_prop(&self.env, fb, prop, v.value.clone(), Some(v.timestamp)).map_err(|e| KnowledgeBaseError::KBError(format!("Failed to set dynamic property {} for object {}: {:#?}", name, id, e)))?;
                            let fact = self.env.assert_fact(fb).map_err(|e| KnowledgeBaseError::KBError(format!("Failed to assert fact for dynamic property {} of object {}: {}", name, id, e)))?;
                            self.values.entry(class.name.clone()).or_default().entry(id.clone()).or_default().insert(name.clone(), fact);
                        } else {
                            let def = get_default(prop);
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
        Ok(())
    }

    fn add_class(&mut self, object_id: &str, class_name: &str) -> Result<(), KnowledgeBaseError> {
        let object = self.objects.get_mut(object_id).ok_or_else(|| KnowledgeBaseError::ObjectNotFound(object_id.to_owned()))?;
        let class = self.classes.get(class_name).ok_or_else(|| KnowledgeBaseError::ClassNotFound(class_name.to_owned()))?;
        let fb = self.env.fact_builder(&class.name).unwrap().put_symbol("id", object_id).map_err(|e| KnowledgeBaseError::KBError(format!("Failed to set id slot for object {}: {}", object_id, e)))?;
        let fact = self.env.assert_fact(fb).map_err(|e| KnowledgeBaseError::KBError(format!("Failed to assert fact for object {}: {}", object_id, e)))?;
        self.instances.entry(class.name.clone()).or_default().insert(object_id.to_owned(), fact);

        if let Some(static_props) = &class.static_properties {
            for (name, prop) in static_props {
                let fb = self.env.fact_builder(&format!("{}_{}", class.name, name)).map_err(|e| KnowledgeBaseError::KBError(format!("Failed to create fact builder for property {} of object {}: {}", name, object_id, e)))?;
                if let Some(v) = object.properties.as_ref().and_then(|props| props.get(name)) {
                    let fb = set_prop(&self.env, fb, prop, v.clone(), None)?;
                    let fact = self.env.assert_fact(fb).map_err(|e| KnowledgeBaseError::KBError(format!("Failed to assert fact for property {} of object {}: {}", name, object_id, e)))?;
                    self.values.entry(class.name.clone()).or_default().entry(object_id.to_owned()).or_default().insert(name.clone(), fact);
                } else {
                    let def = get_default(prop);
                    let fb = set_prop(&self.env, fb, prop, def.clone(), None)?;
                    let fact = self.env.assert_fact(fb).map_err(|e| KnowledgeBaseError::KBError(format!("Failed to assert fact for default value of property {} of object {}: {}", name, object_id, e)))?;
                    self.values.entry(class.name.clone()).or_default().entry(object_id.to_owned()).or_default().insert(name.clone(), fact);
                }
            }
        }

        if let Some(dynamic_props) = &class.dynamic_properties {
            for (name, prop) in dynamic_props {
                if let Some(v) = object.values.as_ref().and_then(|vals| vals.get(name)) {
                    let fb = self.env.fact_builder(&format!("{}_{}", class.name, name)).map_err(|e| KnowledgeBaseError::KBError(format!("Failed to create fact builder for dynamic property {} of object {}: {}", name, object_id, e)))?;
                    let fb = set_prop(&self.env, fb, prop, v.value.clone(), Some(v.timestamp)).map_err(|e| KnowledgeBaseError::KBError(format!("Failed to set dynamic property {} for object {}: {:#?}", name, object_id, e)))?;
                    let fact = self.env.assert_fact(fb).map_err(|e| KnowledgeBaseError::KBError(format!("Failed to assert fact for dynamic property {} of object {}: {}", name, object_id, e)))?;
                    self.values.entry(class.name.clone()).or_default().entry(object_id.to_owned()).or_default().insert(name.clone(), fact);
                } else {
                    let def = get_default(prop);
                    let fb = self.env.fact_builder(&format!("{}_{}", class.name, name)).map_err(|e| KnowledgeBaseError::KBError(format!("Failed to create fact builder for dynamic property {} of object {}: {}", name, object_id, e)))?;
                    let fb = set_prop(&self.env, fb, prop, def.clone(), Some(Utc::now()))?;
                    let fact = self.env.assert_fact(fb).map_err(|e| KnowledgeBaseError::KBError(format!("Failed to assert fact for default value of dynamic property {} of object {}: {}", name, object_id, e)))?;
                    self.values.entry(class.name.clone()).or_default().entry(object_id.to_owned()).or_default().insert(name.clone(), fact);
                }
            }
        }
        Ok(())
    }

    fn set_properties(&mut self, object_id: &str, properties: HashMap<String, Value>) -> Result<(), KnowledgeBaseError> {
        let object = self.objects.get_mut(object_id).ok_or_else(|| KnowledgeBaseError::ObjectNotFound(object_id.to_owned()))?;
        for class_name in &object.classes {
            if let Some(class) = self.classes.get(class_name) {
                if let Some(static_props) = &class.static_properties {
                    for (name, prop) in static_props {
                        if let Some(v) = properties.get(name) {
                            object.properties.get_or_insert_with(HashMap::new).insert(name.clone(), v.clone());
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
        Ok(())
    }

    fn add_values(&mut self, object_id: &str, values: HashMap<String, Value>, timestamp: DateTime<Utc>) -> Result<(), KnowledgeBaseError> {
        let object = self.objects.get_mut(object_id).ok_or_else(|| KnowledgeBaseError::ObjectNotFound(object_id.to_owned()))?;
        for class_name in &object.classes {
            if let Some(class) = self.classes.get(class_name) {
                if let Some(dynamic_props) = &class.dynamic_properties {
                    for (name, prop) in dynamic_props {
                        if let Some(v) = values.get(name) {
                            object.values.get_or_insert_with(HashMap::new).insert(name.clone(), TimedValue { value: v.clone(), timestamp });
                            let fact = self.values.get(class_name).and_then(|objs| objs.get(object_id)).and_then(|props| props.get(name)).ok_or_else(|| KnowledgeBaseError::ObjectNotFound(format!("Fact for dynamic property {} of object {} of class {} not found", name, object_id, class_name)))?;
                            let fm = self.env.fact_modifier(fact).map_err(|e| KnowledgeBaseError::KBError(format!("Failed to create fact modifier for object {}: {}", object_id, e)))?;
                            let fm = update_prop(&self.env, fm, prop, v.clone(), Some(timestamp))?;
                            self.env.modify_fact(fm).map_err(|e| KnowledgeBaseError::KBError(format!("Failed to modify fact for dynamic property {} of object {}: {}", name, object_id, e)))?;
                        }
                    }
                }
            } else {
                return Err(KnowledgeBaseError::ClassNotFound(format!("Class {} not found for object {}", class_name, object_id)));
            }
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
        if self.rules.contains_key(&rule_name) {
            return Err(KnowledgeBaseError::RuleAlreadyExists(rule_name.clone()));
        }
        self.env.build(rule.content.as_str()).map_err(|e| KnowledgeBaseError::KBError(format!("Failed to create rule in CLIPS: {}", e)))?;

        self.rules.insert(rule_name.clone(), rule);
        Ok(())
    }

    fn run(&mut self) -> Result<(), KnowledgeBaseError> {
        self.env.run(-1);
        Ok(())
    }

    fn set_callback(&self, cb: impl Fn(KnowledgeBaseEvent) + 'static) {
        self.callback.replace(Some(Box::new(cb)));
    }
}

fn prop_deftemplate(class: &Class, name: &str, property: &Property, is_static: bool) -> String {
    let mut def = format!("(deftemplate {}_{} (slot id (type SYMBOL))", class.name, name);
    match property {
        Property::Bool { default } => {
            def.push_str(" (slot value (type SYMBOL) (allowed-symbols TRUE FALSE nil)");
            if let Some(def_val) = default {
                def.push_str(&format!(" (default {})", if *def_val { "TRUE" } else { "FALSE" }));
            } else {
                def.push_str(" (default nil)");
            }
            def.push(')');
            if !is_static {
                def.push_str(" (slot time (type INTEGER))");
            }
            def.push(')');
            def
        }
        Property::Int { default, min, max } => {
            def.push_str(" (slot value (type INTEGER SYMBOL) (allowed-symbols nil)");
            if let Some(def_val) = default {
                def.push_str(&format!(" (default {})", def_val));
            } else {
                def.push_str(" (default nil)");
            }
            if min.is_some() || max.is_some() {
                let min_str = min.map(|v| v.to_string()).unwrap_or("?VARIABLE".to_owned());
                let max_str = max.map(|v| v.to_string()).unwrap_or("?VARIABLE".to_owned());
                def.push_str(&format!(" (range {} {})", min_str, max_str));
            }
            def.push(')');
            if !is_static {
                def.push_str(" (slot time (type INTEGER))");
            }
            def.push(')');
            def
        }
        Property::Float { default, min, max } => {
            def.push_str(" (slot value (type FLOAT SYMBOL) (allowed-symbols nil)");
            if let Some(def_val) = default {
                let def_str = def_val.to_string();
                let def_str = if def_str.contains('.') { def_str } else { format!("{}.0", def_str) };
                def.push_str(&format!(" (default {})", def_str));
            } else {
                def.push_str(" (default nil)");
            }
            if min.is_some() || max.is_some() {
                let min_str = min
                    .map(|v| {
                        let s = v.to_string();
                        if s.contains('.') { s } else { format!("{}.0", s) }
                    })
                    .unwrap_or("?VARIABLE".to_owned());
                let max_str = max
                    .map(|v| {
                        let s = v.to_string();
                        if s.contains('.') { s } else { format!("{}.0", s) }
                    })
                    .unwrap_or("?VARIABLE".to_owned());
                def.push_str(&format!(" (range {} {})", min_str, max_str));
            }
            def.push(')');
            if !is_static {
                def.push_str(" (slot time (type INTEGER))");
            }
            def.push(')');
            def
        }
        Property::String { default } => {
            def.push_str(" (slot value (type STRING SYMBOL) (allowed-symbols nil)");
            if let Some(def_val) = default {
                def.push_str(&format!(" (default \"{}\")", def_val));
            } else {
                def.push_str(" (default nil)");
            }
            def.push(')');
            if !is_static {
                def.push_str(" (slot time (type INTEGER))");
            }
            def.push(')');
            def
        }
        Property::Symbol { default, allowed_values } => {
            def.push_str(" (slot value (type SYMBOL)");
            if let Some(allowed) = allowed_values {
                def.push_str(" (allowed-symbols nil");
                for v in allowed {
                    def.push_str(&format!(" {}", v));
                }
                def.push(')');
            } else {
                def.push_str(" (allowed-symbols nil)");
            }

            if let Some(def_val) = default {
                def.push_str(&format!(" (default {})", def_val));
            } else {
                def.push_str(" (default nil)");
            }
            def.push(')');
            if !is_static {
                def.push_str(" (slot time (type INTEGER))");
            }
            def.push(')');
            def
        }
        Property::Object { default, .. } => {
            def.push_str(" (slot value (type SYMBOL)");
            if let Some(def_val) = default {
                def.push_str(&format!(" (default {})", def_val));
            } else {
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
            def.push_str(" (multislot value (type SYMBOL) (allowed-symbols TRUE FALSE nil)");
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
            def.push_str(" (multislot value (type INTEGER SYMBOL) (allowed-symbols nil)");
            if let Some(def_val) = default {
                let def_str = def_val.iter().map(|i| i.to_string()).collect::<Vec<_>>().join(" ");
                def.push_str(&format!(" (default {})", def_str));
            }
            if min.is_some() || max.is_some() {
                let min_str = min.map(|v| v.to_string()).unwrap_or("?VARIABLE".to_owned());
                let max_str = max.map(|v| v.to_string()).unwrap_or("?VARIABLE".to_owned());
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
            def.push_str(" (multislot value (type FLOAT SYMBOL) (allowed-symbols nil)");
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
                    .unwrap_or("?VARIABLE".to_owned());
                let max_str = max
                    .map(|v| {
                        let s = v.to_string();
                        if s.contains('.') { s } else { format!("{}.0", s) }
                    })
                    .unwrap_or("?VARIABLE".to_owned());
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
            def.push_str(" (multislot value (type STRING SYMBOL) (allowed-symbols nil)");
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
                def.push_str(" (allowed-symbols nil");
                for v in allowed {
                    def.push_str(&format!(" {}", v));
                }
                def.push(')');
            } else {
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
        Property::ObjectArray { default, .. } => {
            def.push_str(" (multislot value (type SYMBOL)");
            if let Some(def_val) = default {
                let def_str = def_val.iter().map(|o| o.as_str()).collect::<Vec<_>>().join(" ");
                def.push_str(&format!(" (default {})", def_str));
            } else {
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
        (Property::Bool { .. }, Value::Null) => fb.put_symbol("value", "nil").map_err(|e| KnowledgeBaseError::KBError(format!("Failed to set null value for bool property: {}", e))),
        (Property::Int { .. }, Value::Int(i)) => fb.put_int("value", i).map_err(|e| KnowledgeBaseError::KBError(format!("Failed to set int property value: {}", e))),
        (Property::Int { .. }, Value::Null) => fb.put_symbol("value", "nil").map_err(|e| KnowledgeBaseError::KBError(format!("Failed to set null value for int property: {}", e))),
        (Property::Float { .. }, Value::Float(f)) => fb.put_float("value", f).map_err(|e| KnowledgeBaseError::KBError(format!("Failed to set float property value: {}", e))),
        (Property::Float { .. }, Value::Null) => fb.put_symbol("value", "nil").map_err(|e| KnowledgeBaseError::KBError(format!("Failed to set null value for float property: {}", e))),
        (Property::String { .. }, Value::String(s)) => fb.put_string("value", s.as_str()).map_err(|e| KnowledgeBaseError::KBError(format!("Failed to set string property value: {}", e))),
        (Property::String { .. }, Value::Null) => fb.put_symbol("value", "nil").map_err(|e| KnowledgeBaseError::KBError(format!("Failed to set null value for string property: {}", e))),
        (Property::Symbol { .. }, Value::Symbol(s)) => fb.put_symbol("value", s.as_str()).map_err(|e| KnowledgeBaseError::KBError(format!("Failed to set symbol property value: {}", e))),
        (Property::Symbol { .. }, Value::Null) => fb.put_symbol("value", "nil").map_err(|e| KnowledgeBaseError::KBError(format!("Failed to set null value for symbol property: {}", e))),
        (Property::Object { .. }, Value::Object(o)) => fb.put_symbol("value", o.as_str()).map_err(|e| KnowledgeBaseError::KBError(format!("Failed to set object property value: {}", e))),
        (Property::Object { .. }, Value::Null) => fb.put_symbol("value", "nil").map_err(|e| KnowledgeBaseError::KBError(format!("Failed to set null value for object property: {}", e))),
        (Property::BoolArray { .. }, Value::BoolArray(arr)) => {
            let builder = env.multifield_builder(arr.len()).map_err(|e| KnowledgeBaseError::KBError(format!("Failed to create multifield for bool array: {}", e)))?;
            let builder = arr.iter().fold(builder, |bld, &b| bld.put_symbol(if b { "TRUE" } else { "FALSE" }));
            fb.put_multifield("value", builder.create()).map_err(|e| KnowledgeBaseError::KBError(format!("Failed to set bool array property value: {}", e)))
        }
        (Property::BoolArray { .. }, Value::Null) => fb.put_symbol("value", "nil").map_err(|e| KnowledgeBaseError::KBError(format!("Failed to set null value for bool array property: {}", e))),
        (Property::IntArray { .. }, Value::IntArray(arr)) => {
            let builder = env.multifield_builder(arr.len()).map_err(|e| KnowledgeBaseError::KBError(format!("Failed to create multifield for int array: {}", e)))?;
            let builder = arr.iter().fold(builder, |bld, &i| bld.put_int(i));
            fb.put_multifield("value", builder.create()).map_err(|e| KnowledgeBaseError::KBError(format!("Failed to set int array property value: {}", e)))
        }
        (Property::IntArray { .. }, Value::Null) => fb.put_symbol("value", "nil").map_err(|e| KnowledgeBaseError::KBError(format!("Failed to set null value for int array property: {}", e))),
        (Property::FloatArray { .. }, Value::FloatArray(arr)) => {
            let builder = env.multifield_builder(arr.len()).map_err(|e| KnowledgeBaseError::KBError(format!("Failed to create multifield for float array: {}", e)))?;
            let builder = arr.iter().fold(builder, |bld, &f| bld.put_float(f));
            fb.put_multifield("value", builder.create()).map_err(|e| KnowledgeBaseError::KBError(format!("Failed to set float array property value: {}", e)))
        }
        (Property::FloatArray { .. }, Value::Null) => fb.put_symbol("value", "nil").map_err(|e| KnowledgeBaseError::KBError(format!("Failed to set null value for float array property: {}", e))),
        (Property::StringArray { .. }, Value::StringArray(arr)) => {
            let builder = env.multifield_builder(arr.len()).map_err(|e| KnowledgeBaseError::KBError(format!("Failed to create multifield for string array: {}", e)))?;
            let builder = arr.iter().fold(builder, |bld, s| bld.put_string(s.as_str()));
            fb.put_multifield("value", builder.create()).map_err(|e| KnowledgeBaseError::KBError(format!("Failed to set string array property value: {}", e)))
        }
        (Property::StringArray { .. }, Value::Null) => fb.put_symbol("value", "nil").map_err(|e| KnowledgeBaseError::KBError(format!("Failed to set null value for string array property: {}", e))),
        (Property::SymbolArray { .. }, Value::StringArray(arr)) => {
            let builder = env.multifield_builder(arr.len()).map_err(|e| KnowledgeBaseError::KBError(format!("Failed to create multifield for symbol array: {}", e)))?;
            let builder = arr.iter().fold(builder, |bld, s| bld.put_symbol(s.as_str()));
            fb.put_multifield("value", builder.create()).map_err(|e| KnowledgeBaseError::KBError(format!("Failed to set symbol array property value: {}", e)))
        }
        (Property::SymbolArray { .. }, Value::Null) => fb.put_symbol("value", "nil").map_err(|e| KnowledgeBaseError::KBError(format!("Failed to set null value for symbol array property: {}", e))),
        (Property::ObjectArray { .. }, Value::StringArray(arr)) => {
            let builder = env.multifield_builder(arr.len()).map_err(|e| KnowledgeBaseError::KBError(format!("Failed to create multifield for object array: {}", e)))?;
            let builder = arr.iter().fold(builder, |bld, o| bld.put_symbol(o.as_str()));
            fb.put_multifield("value", builder.create()).map_err(|e| KnowledgeBaseError::KBError(format!("Failed to set object array property value: {}", e)))
        }
        (Property::ObjectArray { .. }, Value::Null) => fb.put_symbol("value", "nil").map_err(|e| KnowledgeBaseError::KBError(format!("Failed to set null value for object array property: {}", e))),
        _ => Err(KnowledgeBaseError::KBError("Property type and value type do not match".to_owned())),
    };
    if let Some(t) = time { builder.and_then(|fb| fb.put_int("time", t.timestamp()).map_err(|e| KnowledgeBaseError::KBError(format!("Failed to set time slot for property value: {}", e)))) } else { builder }
}

fn update_prop(env: &Environment, fm: FactModifier, property: &Property, value: Value, time: Option<DateTime<Utc>>) -> Result<FactModifier, KnowledgeBaseError> {
    let modifier = match (property, value) {
        (Property::Bool { .. }, Value::Bool(b)) => fm.put_symbol("value", if b { "TRUE" } else { "FALSE" }).map_err(|e| KnowledgeBaseError::KBError(format!("Failed to set bool property value: {}", e))),
        (Property::Bool { .. }, Value::Null) => fm.put_symbol("value", "nil").map_err(|e| KnowledgeBaseError::KBError(format!("Failed to set null value for bool property: {}", e))),
        (Property::Int { .. }, Value::Int(i)) => fm.put_int("value", i).map_err(|e| KnowledgeBaseError::KBError(format!("Failed to set int property value: {}", e))),
        (Property::Int { .. }, Value::Null) => fm.put_symbol("value", "nil").map_err(|e| KnowledgeBaseError::KBError(format!("Failed to set null value for int property: {}", e))),
        (Property::Float { .. }, Value::Float(f)) => fm.put_float("value", f).map_err(|e| KnowledgeBaseError::KBError(format!("Failed to set float property value: {}", e))),
        (Property::Float { .. }, Value::Null) => fm.put_symbol("value", "nil").map_err(|e| KnowledgeBaseError::KBError(format!("Failed to set null value for float property: {}", e))),
        (Property::String { .. }, Value::String(s)) => fm.put_string("value", s.as_str()).map_err(|e| KnowledgeBaseError::KBError(format!("Failed to set string property value: {}", e))),
        (Property::String { .. }, Value::Null) => fm.put_symbol("value", "nil").map_err(|e| KnowledgeBaseError::KBError(format!("Failed to set null value for string property: {}", e))),
        (Property::Symbol { .. }, Value::Symbol(s)) => fm.put_symbol("value", s.as_str()).map_err(|e| KnowledgeBaseError::KBError(format!("Failed to set symbol property value: {}", e))),
        (Property::Symbol { .. }, Value::Null) => fm.put_symbol("value", "nil").map_err(|e| KnowledgeBaseError::KBError(format!("Failed to set null value for symbol property: {}", e))),
        (Property::Object { .. }, Value::Object(o)) => fm.put_symbol("value", o.as_str()).map_err(|e| KnowledgeBaseError::KBError(format!("Failed to set object property value: {}", e))),
        (Property::Object { .. }, Value::Null) => fm.put_symbol("value", "nil").map_err(|e| KnowledgeBaseError::KBError(format!("Failed to set null value for object property: {}", e))),
        (Property::BoolArray { .. }, Value::BoolArray(arr)) => {
            let builder = env.multifield_builder(arr.len()).map_err(|e| KnowledgeBaseError::KBError(format!("Failed to create multifield for bool array: {}", e)))?;
            let builder = arr.iter().fold(builder, |bld, &b| bld.put_symbol(if b { "TRUE" } else { "FALSE" }));
            fm.put_multifield("value", builder.create()).map_err(|e| KnowledgeBaseError::KBError(format!("Failed to set bool array property value: {}", e)))
        }
        (Property::BoolArray { .. }, Value::Null) => fm.put_symbol("value", "nil").map_err(|e| KnowledgeBaseError::KBError(format!("Failed to set null value for bool array property: {}", e))),
        (Property::IntArray { .. }, Value::IntArray(arr)) => {
            let builder = env.multifield_builder(arr.len()).map_err(|e| KnowledgeBaseError::KBError(format!("Failed to create multifield for int array: {}", e)))?;
            let builder = arr.iter().fold(builder, |bld, &i| bld.put_int(i));
            fm.put_multifield("value", builder.create()).map_err(|e| KnowledgeBaseError::KBError(format!("Failed to set int array property value: {}", e)))
        }
        (Property::IntArray { .. }, Value::Null) => fm.put_symbol("value", "nil").map_err(|e| KnowledgeBaseError::KBError(format!("Failed to set null value for int array property: {}", e))),
        (Property::FloatArray { .. }, Value::FloatArray(arr)) => {
            let builder = env.multifield_builder(arr.len()).map_err(|e| KnowledgeBaseError::KBError(format!("Failed to create multifield for float array: {}", e)))?;
            let builder = arr.iter().fold(builder, |bld, &f| bld.put_float(f));
            fm.put_multifield("value", builder.create()).map_err(|e| KnowledgeBaseError::KBError(format!("Failed to set float array property value: {}", e)))
        }
        (Property::FloatArray { .. }, Value::Null) => fm.put_symbol("value", "nil").map_err(|e| KnowledgeBaseError::KBError(format!("Failed to set null value for float array property: {}", e))),
        (Property::StringArray { .. }, Value::StringArray(arr)) => {
            let builder = env.multifield_builder(arr.len()).map_err(|e| KnowledgeBaseError::KBError(format!("Failed to create multifield for string array: {}", e)))?;
            let builder = arr.iter().fold(builder, |bld, s| bld.put_string(s.as_str()));
            fm.put_multifield("value", builder.create()).map_err(|e| KnowledgeBaseError::KBError(format!("Failed to set string array property value: {}", e)))
        }
        (Property::StringArray { .. }, Value::Null) => fm.put_symbol("value", "nil").map_err(|e| KnowledgeBaseError::KBError(format!("Failed to set null value for string array property: {}", e))),
        (Property::SymbolArray { .. }, Value::StringArray(arr)) => {
            let builder = env.multifield_builder(arr.len()).map_err(|e| KnowledgeBaseError::KBError(format!("Failed to create multifield for symbol array: {}", e)))?;
            let builder = arr.iter().fold(builder, |bld, s| bld.put_symbol(s.as_str()));
            fm.put_multifield("value", builder.create()).map_err(|e| KnowledgeBaseError::KBError(format!("Failed to set symbol array property value: {}", e)))
        }
        (Property::SymbolArray { .. }, Value::Null) => fm.put_symbol("value", "nil").map_err(|e| KnowledgeBaseError::KBError(format!("Failed to set null value for symbol array property: {}", e))),
        (Property::ObjectArray { .. }, Value::StringArray(arr)) => {
            let builder = env.multifield_builder(arr.len()).map_err(|e| KnowledgeBaseError::KBError(format!("Failed to create multifield for object array: {}", e)))?;
            let builder = arr.iter().fold(builder, |bld, o| bld.put_symbol(o.as_str()));
            fm.put_multifield("value", builder.create()).map_err(|e| KnowledgeBaseError::KBError(format!("Failed to set object array property value: {}", e)))
        }
        (Property::ObjectArray { .. }, Value::Null) => fm.put_symbol("value", "nil").map_err(|e| KnowledgeBaseError::KBError(format!("Failed to set null value for object array property: {}", e))),
        _ => Err(KnowledgeBaseError::KBError("Property type and value type do not match".to_owned())),
    };
    if let Some(t) = time { modifier.and_then(|fm| fm.put_int("time", t.timestamp()).map_err(|e| KnowledgeBaseError::KBError(format!("Failed to set time slot for property value: {}", e)))) } else { modifier }
}

fn get_default(property: &Property) -> Value {
    match property {
        Property::Bool { default, .. } => default.map(Value::Bool).unwrap_or(Value::Null),
        Property::Int { default, .. } => default.map(Value::Int).unwrap_or(Value::Null),
        Property::Float { default, .. } => default.map(Value::Float).unwrap_or(Value::Null),
        Property::String { default, .. } => default.clone().map(Value::String).unwrap_or(Value::Null),
        Property::Symbol { default, .. } => default.clone().map(Value::Symbol).unwrap_or(Value::Null),
        Property::Object { default, .. } => default.clone().map(Value::Object).unwrap_or(Value::Null),
        Property::BoolArray { default } => default.clone().map(Value::BoolArray).unwrap_or(Value::Null),
        Property::IntArray { default, .. } => default.clone().map(Value::IntArray).unwrap_or(Value::Null),
        Property::FloatArray { default, .. } => default.clone().map(Value::FloatArray).unwrap_or(Value::Null),
        Property::StringArray { default } => default.clone().map(Value::StringArray).unwrap_or(Value::Null),
        Property::SymbolArray { default, .. } => default.clone().map(Value::StringArray).unwrap_or(Value::Null),
        Property::ObjectArray { default, .. } => default.clone().map(Value::StringArray).unwrap_or(Value::Null),
    }
}

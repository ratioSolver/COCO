use crate::{
    kb::{KnowledgeBase, KnowledgeBaseError},
    model::{Class, Object, Property, Rule, Value},
};
use chrono::{DateTime, Utc};
use clips::{ClipsValue, Environment, Fact, FactBuilder, FactModifier, Type};
use std::{collections::HashMap, sync::Mutex};

pub struct CLIPSKnowledgeBase {
    env: Mutex<Environment>,
    classes: HashMap<String, Class>,
    objects: HashMap<String, Object>,
    rules: HashMap<String, Rule>,
    instances: HashMap<String, HashMap<String, Fact>>,
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
        let mut env = Environment::new().expect("Failed to create CLIPS environment");
        env.add_udf("add-data", None, 3, 4, vec![Type(Type::SYMBOL), Type(Type::MULTIFIELD), Type(Type::MULTIFIELD), Type(Type::INTEGER)], |_env, _ctx| ClipsValue::Void()).expect("Failed to add UDF to CLIPS environment");
        env.add_udf("add-class", None, 2, 2, vec![Type(Type::SYMBOL), Type(Type::SYMBOL)], |_env, _ctx| ClipsValue::Void()).expect("Failed to add UDF to CLIPS environment");
        Self {
            env: Mutex::new(env),
            classes: HashMap::new(),
            objects: HashMap::new(),
            rules: HashMap::new(),
            instances: HashMap::new(),
        }
    }
}

impl KnowledgeBase for CLIPSKnowledgeBase {
    fn get_classes(&self) -> Vec<&Class> {
        self.classes.values().collect()
    }

    fn get_class(&self, name: &str) -> Option<&Class> {
        self.classes.get(name)
    }

    fn create_class(&mut self, class: &Class) -> Result<(), KnowledgeBaseError> {
        if self.classes.contains_key(&class.name) {
            return Err(KnowledgeBaseError::ClassAlreadyExists(class.name.clone()));
        }
        let mut env = self.env.lock().map_err(|e| KnowledgeBaseError::KBError(format!("Failed to lock CLIPS environment: {}", e)))?;
        env.build(format!("(deftemplate {} (slot id (type SYMBOL)))", class.name).as_str()).map_err(|e| KnowledgeBaseError::KBError(format!("Failed to create class in CLIPS: {}", e)))?;
        if let Some(static_props) = &class.static_properties {
            for (name, prop) in static_props {
                env.build(prop_deftemplate(class, name, prop, true).as_str()).map_err(|e| KnowledgeBaseError::KBError(format!("Failed to create static property {} for class {} in CLIPS: {}", name, class.name, e)))?;
            }
        }
        if let Some(dynamic_props) = &class.dynamic_properties {
            for (name, prop) in dynamic_props {
                env.build(prop_deftemplate(class, name, prop, false).as_str()).map_err(|e| KnowledgeBaseError::KBError(format!("Failed to create dynamic property {} for class {} in CLIPS: {}", name, class.name, e)))?;
            }
        }
        self.classes.insert(class.name.clone(), class.clone());
        Ok(())
    }

    fn get_objects(&self) -> Vec<&Object> {
        self.objects.values().collect()
    }

    fn get_object(&self, id: &str) -> Option<&Object> {
        self.objects.get(id)
    }

    fn create_object(&mut self, object: &Object) -> Result<(), KnowledgeBaseError> {
        if let Some(id) = &object.id {
            if self.objects.contains_key(id) {
                return Err(KnowledgeBaseError::ObjectAlreadyExists(id.clone()));
            }
            let mut env = self.env.lock().map_err(|e| KnowledgeBaseError::KBError(format!("Failed to lock CLIPS environment: {}", e)))?;
            for class_name in &object.classes {
                if let Some(class) = self.classes.get(class_name) {
                    let fb = env.fact_builder(&class.name).unwrap().put_symbol("id", id).map_err(|e| KnowledgeBaseError::KBError(format!("Failed to set id slot for object {}: {}", id, e)))?;
                    let fact = env.assert_fact(fb).map_err(|e| KnowledgeBaseError::KBError(format!("Failed to assert fact for object {}: {}", id, e)))?;
                    self.instances.entry(class.name.clone()).or_default().insert(id.clone(), fact);

                    if let Some(static_props) = &class.static_properties {
                        for (name, prop) in static_props {
                            let fb = env.fact_builder(&format!("{}_{}", class.name, name)).map_err(|e| KnowledgeBaseError::KBError(format!("Failed to create fact builder for property {} of object {}: {}", name, id, e)))?;
                            if let Some(v) = object.properties.as_ref().and_then(|props| props.get(name)) {
                                let fb = set_prop(&env, fb, prop, v.clone(), None)?;
                                env.assert_fact(fb).map_err(|e| KnowledgeBaseError::KBError(format!("Failed to assert fact for property {} of object {}: {}", name, id, e)))?;
                            } else if let Some(def) = get_default(prop) {
                                let fb = set_prop(&env, fb, prop, def.clone(), None)?;
                                env.assert_fact(fb).map_err(|e| KnowledgeBaseError::KBError(format!("Failed to assert fact for default value of property {} of object {}: {}", name, id, e)))?;
                            }
                        }
                    }

                    if let Some(dynamic_props) = &class.dynamic_properties {
                        for (name, prop) in dynamic_props {
                            if let Some(v) = object.values.as_ref().and_then(|vals| vals.get(name)) {
                                let fb = env.fact_builder(&format!("{}_{}", class.name, name)).map_err(|e| KnowledgeBaseError::KBError(format!("Failed to create fact builder for dynamic property {} of object {}: {}", name, id, e)))?;
                                let fb = set_prop(&env, fb, prop, v.0.clone(), Some(v.1))?;
                                env.assert_fact(fb).map_err(|e| KnowledgeBaseError::KBError(format!("Failed to assert fact for dynamic property {} of object {}: {}", name, id, e)))?;
                            } else if let Some(def) = get_default(prop) {
                                // If the object doesn't have a value for this dynamic property, but there is a default, we should use the default
                                let fb = env.fact_builder(&format!("{}_{}", class.name, name)).map_err(|e| KnowledgeBaseError::KBError(format!("Failed to create fact builder for dynamic property {} of object {}: {}", name, id, e)))?;
                                let fb = set_prop(&env, fb, prop, def.clone(), Some(Utc::now()))?;
                                env.assert_fact(fb).map_err(|e| KnowledgeBaseError::KBError(format!("Failed to assert fact for default value of dynamic property {} of object {}: {}", name, id, e)))?;
                            }
                        }
                    }
                } else {
                    return Err(KnowledgeBaseError::ClassNotFound(format!("Class {} not found for object {}", class_name, id)));
                }
            }
            self.objects.insert(id.clone(), object.clone());
        } else {
            return Err(KnowledgeBaseError::ObjectNotFound("Object must have an ID".to_string()));
        }
        Ok(())
    }

    fn add_class(&mut self, object: &Object, class: &Class) -> Result<(), KnowledgeBaseError> {
        if let Some(id) = &object.id {
            if !self.objects.contains_key(id) {
                return Err(KnowledgeBaseError::ObjectNotFound(id.clone()));
            }
            if !self.classes.contains_key(&class.name) {
                return Err(KnowledgeBaseError::ClassNotFound(class.name.clone()));
            }
            let mut env = self.env.lock().map_err(|e| KnowledgeBaseError::KBError(format!("Failed to lock CLIPS environment: {}", e)))?;
            let fb = env.fact_builder(&class.name).unwrap().put_symbol("id", id).map_err(|e| KnowledgeBaseError::KBError(format!("Failed to set id slot for object {}: {}", id, e)))?;
            let fact = env.assert_fact(fb).map_err(|e| KnowledgeBaseError::KBError(format!("Failed to assert fact for object {}: {}", id, e)))?;
            self.instances.entry(class.name.clone()).or_default().insert(id.clone(), fact);

            if let Some(static_props) = &class.static_properties {
                for (name, prop) in static_props {
                    let fb = env.fact_builder(&format!("{}_{}", class.name, name)).map_err(|e| KnowledgeBaseError::KBError(format!("Failed to create fact builder for property {} of object {}: {}", name, id, e)))?;
                    if let Some(v) = object.properties.as_ref().and_then(|props| props.get(name)) {
                        let fb = set_prop(&env, fb, prop, v.clone(), None)?;
                        env.assert_fact(fb).map_err(|e| KnowledgeBaseError::KBError(format!("Failed to assert fact for property {} of object {}: {}", name, id, e)))?;
                    } else if let Some(def) = get_default(prop) {
                        let fb = set_prop(&env, fb, prop, def.clone(), None)?;
                        env.assert_fact(fb).map_err(|e| KnowledgeBaseError::KBError(format!("Failed to assert fact for default value of property {} of object {}: {}", name, id, e)))?;
                    }
                }
            }

            if let Some(dynamic_props) = &class.dynamic_properties {
                for (name, prop) in dynamic_props {
                    if let Some(v) = object.values.as_ref().and_then(|vals| vals.get(name)) {
                        let fb = env.fact_builder(&format!("{}_{}", class.name, name)).map_err(|e| KnowledgeBaseError::KBError(format!("Failed to create fact builder for dynamic property {} of object {}: {}", name, id, e)))?;
                        let fb = set_prop(&env, fb, prop, v.0.clone(), Some(v.1))?;
                        env.assert_fact(fb).map_err(|e| KnowledgeBaseError::KBError(format!("Failed to assert fact for dynamic property {} of object {}: {}", name, id, e)))?;
                    } else if let Some(def) = get_default(prop) {
                        // If the object doesn't have a value for this dynamic property, but there is a default, we should use the default
                        let fb = env.fact_builder(&format!("{}_{}", class.name, name)).map_err(|e| KnowledgeBaseError::KBError(format!("Failed to create fact builder for dynamic property {} of object {}: {}", name, id, e)))?;
                        let fb = set_prop(&env, fb, prop, def.clone(), Some(Utc::now()))?;
                        env.assert_fact(fb).map_err(|e| KnowledgeBaseError::KBError(format!("Failed to assert fact for default value of dynamic property {} of object {}: {}", name, id, e)))?;
                    }
                }
            }
            Ok(())
        } else {
            Err(KnowledgeBaseError::ObjectNotFound("Object must have an ID".to_string()))
        }
    }

    fn set_properties(&mut self, object: &Object, properties: HashMap<String, Value>) -> Result<(), KnowledgeBaseError> {
        if let Some(id) = &object.id {
            if !self.objects.contains_key(id) {
                return Err(KnowledgeBaseError::ObjectNotFound(id.clone()));
            }
            let mut env = self.env.lock().map_err(|e| KnowledgeBaseError::KBError(format!("Failed to lock CLIPS environment: {}", e)))?;
            for class_name in &object.classes {
                if let Some(class) = self.classes.get(class_name) {
                    if let Some(static_props) = &class.static_properties {
                        for (name, prop) in static_props {
                            if let Some(v) = properties.get(name) {
                                let fact = self.instances.get(class_name).and_then(|insts| insts.get(id)).ok_or_else(|| KnowledgeBaseError::ObjectNotFound(format!("Instance of class {} for object {} not found", class_name, id)))?;
                                let fm = env.fact_modifier(fact).map_err(|e| KnowledgeBaseError::KBError(format!("Failed to create fact modifier for object {}: {}", id, e)))?;
                                let fm = update_prop(&env, fm, prop, v.clone(), None)?;
                                let fact = env.modify_fact(fm).map_err(|e| KnowledgeBaseError::KBError(format!("Failed to modify fact for property {} of object {}: {}", name, id, e)))?;
                                self.instances.get_mut(class_name).and_then(|insts| insts.insert(id.clone(), fact));
                            }
                        }
                    }
                } else {
                    return Err(KnowledgeBaseError::ClassNotFound(format!("Class {} not found for object {}", class_name, id)));
                }
            }
            Ok(())
        } else {
            Err(KnowledgeBaseError::ObjectNotFound("Object must have an ID".to_string()))
        }
    }

    fn add_values(&mut self, object: &Object, values: HashMap<String, Value>, date_time: DateTime<Utc>) -> Result<(), KnowledgeBaseError> {
        if let Some(id) = &object.id {
            if !self.objects.contains_key(id) {
                return Err(KnowledgeBaseError::ObjectNotFound(id.clone()));
            }
            let mut env = self.env.lock().map_err(|e| KnowledgeBaseError::KBError(format!("Failed to lock CLIPS environment: {}", e)))?;
            for class_name in &object.classes {
                if let Some(class) = self.classes.get(class_name) {
                    if let Some(dynamic_props) = &class.dynamic_properties {
                        for (name, prop) in dynamic_props {
                            if let Some(v) = values.get(name) {
                                let fact = self.instances.get(class_name).and_then(|insts| insts.get(id)).ok_or_else(|| KnowledgeBaseError::ObjectNotFound(format!("Instance of class {} for object {} not found", class_name, id)))?;
                                let fm = env.fact_modifier(fact).map_err(|e| KnowledgeBaseError::KBError(format!("Failed to create fact modifier for object {}: {}", id, e)))?;
                                let fm = update_prop(&env, fm, prop, v.clone(), Some(date_time))?;
                                let fact = env.modify_fact(fm).map_err(|e| KnowledgeBaseError::KBError(format!("Failed to modify fact for dynamic property {} of object {}: {}", name, id, e)))?;
                                self.instances.get_mut(class_name).and_then(|insts| insts.insert(id.clone(), fact));
                            }
                        }
                    }
                } else {
                    return Err(KnowledgeBaseError::ClassNotFound(format!("Class {} not found for object {}", class_name, id)));
                }
            }
            Ok(())
        } else {
            Err(KnowledgeBaseError::ObjectNotFound("Object must have an ID".to_string()))
        }
    }

    fn get_rules(&self) -> Vec<&Rule> {
        self.rules.values().collect()
    }

    fn get_rule(&self, name: &str) -> Option<&Rule> {
        self.rules.get(name)
    }

    fn create_rule(&mut self, rule: &Rule) -> Result<(), KnowledgeBaseError> {
        if self.rules.contains_key(&rule.name) {
            return Err(KnowledgeBaseError::KBError(format!("Rule {} already exists", rule.name)));
        }
        let mut env = self.env.lock().map_err(|e| KnowledgeBaseError::KBError(format!("Failed to lock CLIPS environment: {}", e)))?;
        env.build(rule.content.as_str()).map_err(|e| KnowledgeBaseError::KBError(format!("Failed to create rule in CLIPS: {}", e)))?;
        self.rules.insert(rule.name.clone(), rule.clone());
        Ok(())
    }

    fn run(&mut self) -> Result<(), KnowledgeBaseError> {
        let mut env = self.env.lock().map_err(|e| KnowledgeBaseError::KBError(format!("Failed to lock CLIPS environment: {}", e)))?;
        env.run(-1);
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
        (Property::Int { .. }, Value::Int(i)) => fb.put_int("value", i).map_err(|e| KnowledgeBaseError::KBError(format!("Failed to set int property value: {}", e))),
        (Property::Float { .. }, Value::Float(f)) => fb.put_float("value", f).map_err(|e| KnowledgeBaseError::KBError(format!("Failed to set float property value: {}", e))),
        (Property::String { .. }, Value::String(s)) => fb.put_string("value", s.as_str()).map_err(|e| KnowledgeBaseError::KBError(format!("Failed to set string property value: {}", e))),
        (Property::Symbol { .. }, Value::Symbol(s)) => fb.put_symbol("value", s.as_str()).map_err(|e| KnowledgeBaseError::KBError(format!("Failed to set symbol property value: {}", e))),
        (Property::Object { .. }, Value::Object(o)) => fb.put_symbol("value", o.as_str()).map_err(|e| KnowledgeBaseError::KBError(format!("Failed to set object property value: {}", e))),
        (Property::BoolArray { .. }, Value::BoolArray(arr)) => {
            let builder = env.multifield_builder(arr.len()).map_err(|e| KnowledgeBaseError::KBError(format!("Failed to create multifield for bool array: {}", e)))?;
            let builder = arr.iter().fold(builder, |bld, &b| bld.put_symbol(if b { "TRUE" } else { "FALSE" }));
            fb.put_multifield("value", builder.create()).map_err(|e| KnowledgeBaseError::KBError(format!("Failed to set bool array property value: {}", e)))
        }
        (Property::IntArray { .. }, Value::IntArray(arr)) => {
            let builder = env.multifield_builder(arr.len()).map_err(|e| KnowledgeBaseError::KBError(format!("Failed to create multifield for int array: {}", e)))?;
            let builder = arr.iter().fold(builder, |bld, &i| bld.put_int(i));
            fb.put_multifield("value", builder.create()).map_err(|e| KnowledgeBaseError::KBError(format!("Failed to set int array property value: {}", e)))
        }
        (Property::FloatArray { .. }, Value::FloatArray(arr)) => {
            let builder = env.multifield_builder(arr.len()).map_err(|e| KnowledgeBaseError::KBError(format!("Failed to create multifield for float array: {}", e)))?;
            let builder = arr.iter().fold(builder, |bld, &f| bld.put_float(f));
            fb.put_multifield("value", builder.create()).map_err(|e| KnowledgeBaseError::KBError(format!("Failed to set float array property value: {}", e)))
        }
        (Property::StringArray { .. }, Value::StringArray(arr)) => {
            let builder = env.multifield_builder(arr.len()).map_err(|e| KnowledgeBaseError::KBError(format!("Failed to create multifield for string array: {}", e)))?;
            let builder = arr.iter().fold(builder, |bld, s| bld.put_string(s.as_str()));
            fb.put_multifield("value", builder.create()).map_err(|e| KnowledgeBaseError::KBError(format!("Failed to set string array property value: {}", e)))
        }
        (Property::SymbolArray { .. }, Value::StringArray(arr)) => {
            let builder = env.multifield_builder(arr.len()).map_err(|e| KnowledgeBaseError::KBError(format!("Failed to create multifield for symbol array: {}", e)))?;
            let builder = arr.iter().fold(builder, |bld, s| bld.put_symbol(s.as_str()));
            fb.put_multifield("value", builder.create()).map_err(|e| KnowledgeBaseError::KBError(format!("Failed to set symbol array property value: {}", e)))
        }
        (Property::ObjectArray { .. }, Value::StringArray(arr)) => {
            let builder = env.multifield_builder(arr.len()).map_err(|e| KnowledgeBaseError::KBError(format!("Failed to create multifield for object array: {}", e)))?;
            let builder = arr.iter().fold(builder, |bld, o| bld.put_symbol(o.as_str()));
            fb.put_multifield("value", builder.create()).map_err(|e| KnowledgeBaseError::KBError(format!("Failed to set object array property value: {}", e)))
        }
        _ => Err(KnowledgeBaseError::KBError("Property type and value type do not match".to_string())),
    };
    if let Some(t) = time { builder.and_then(|fb| fb.put_int("time", t.timestamp()).map_err(|e| KnowledgeBaseError::KBError(format!("Failed to set time slot for property value: {}", e)))) } else { builder }
}

fn update_prop(env: &Environment, fm: FactModifier, property: &Property, value: Value, time: Option<DateTime<Utc>>) -> Result<FactModifier, KnowledgeBaseError> {
    let modifier = match (property, value) {
        (Property::Bool { .. }, Value::Bool(b)) => fm.put_symbol("value", if b { "TRUE" } else { "FALSE" }).map_err(|e| KnowledgeBaseError::KBError(format!("Failed to set bool property value: {}", e))),
        (Property::Int { .. }, Value::Int(i)) => fm.put_int("value", i).map_err(|e| KnowledgeBaseError::KBError(format!("Failed to set int property value: {}", e))),
        (Property::Float { .. }, Value::Float(f)) => fm.put_float("value", f).map_err(|e| KnowledgeBaseError::KBError(format!("Failed to set float property value: {}", e))),
        (Property::String { .. }, Value::String(s)) => fm.put_string("value", s.as_str()).map_err(|e| KnowledgeBaseError::KBError(format!("Failed to set string property value: {}", e))),
        (Property::Symbol { .. }, Value::Symbol(s)) => fm.put_symbol("value", s.as_str()).map_err(|e| KnowledgeBaseError::KBError(format!("Failed to set symbol property value: {}", e))),
        (Property::Object { .. }, Value::Object(o)) => fm.put_symbol("value", o.as_str()).map_err(|e| KnowledgeBaseError::KBError(format!("Failed to set object property value: {}", e))),
        (Property::BoolArray { .. }, Value::BoolArray(arr)) => {
            let builder = env.multifield_builder(arr.len()).map_err(|e| KnowledgeBaseError::KBError(format!("Failed to create multifield for bool array: {}", e)))?;
            let builder = arr.iter().fold(builder, |bld, &b| bld.put_symbol(if b { "TRUE" } else { "FALSE" }));
            fm.put_multifield("value", builder.create()).map_err(|e| KnowledgeBaseError::KBError(format!("Failed to set bool array property value: {}", e)))
        }
        (Property::IntArray { .. }, Value::IntArray(arr)) => {
            let builder = env.multifield_builder(arr.len()).map_err(|e| KnowledgeBaseError::KBError(format!("Failed to create multifield for int array: {}", e)))?;
            let builder = arr.iter().fold(builder, |bld, &i| bld.put_int(i));
            fm.put_multifield("value", builder.create()).map_err(|e| KnowledgeBaseError::KBError(format!("Failed to set int array property value: {}", e)))
        }
        (Property::FloatArray { .. }, Value::FloatArray(arr)) => {
            let builder = env.multifield_builder(arr.len()).map_err(|e| KnowledgeBaseError::KBError(format!("Failed to create multifield for float array: {}", e)))?;
            let builder = arr.iter().fold(builder, |bld, &f| bld.put_float(f));
            fm.put_multifield("value", builder.create()).map_err(|e| KnowledgeBaseError::KBError(format!("Failed to set float array property value: {}", e)))
        }
        (Property::StringArray { .. }, Value::StringArray(arr)) => {
            let builder = env.multifield_builder(arr.len()).map_err(|e| KnowledgeBaseError::KBError(format!("Failed to create multifield for string array: {}", e)))?;
            let builder = arr.iter().fold(builder, |bld, s| bld.put_string(s.as_str()));
            fm.put_multifield("value", builder.create()).map_err(|e| KnowledgeBaseError::KBError(format!("Failed to set string array property value: {}", e)))
        }
        (Property::SymbolArray { .. }, Value::StringArray(arr)) => {
            let builder = env.multifield_builder(arr.len()).map_err(|e| KnowledgeBaseError::KBError(format!("Failed to create multifield for symbol array: {}", e)))?;
            let builder = arr.iter().fold(builder, |bld, s| bld.put_symbol(s.as_str()));
            fm.put_multifield("value", builder.create()).map_err(|e| KnowledgeBaseError::KBError(format!("Failed to set symbol array property value: {}", e)))
        }
        (Property::ObjectArray { .. }, Value::StringArray(arr)) => {
            let builder = env.multifield_builder(arr.len()).map_err(|e| KnowledgeBaseError::KBError(format!("Failed to create multifield for object array: {}", e)))?;
            let builder = arr.iter().fold(builder, |bld, o| bld.put_symbol(o.as_str()));
            fm.put_multifield("value", builder.create()).map_err(|e| KnowledgeBaseError::KBError(format!("Failed to set object array property value: {}", e)))
        }
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

    #[test]
    fn test_defaults() {
        let prop = Property::Int { nullable: Some(true), default: Some(42), min: Some(0), max: Some(100) };
        assert_eq!(get_default(&prop), Some(Value::Int(42)));

        let prop = Property::String { nullable: None, default: Some("hello".to_string()) };
        assert_eq!(get_default(&prop), Some(Value::String("hello".to_string())));

        let prop = Property::BoolArray { nullable: None, default: Some(vec![true, false, true]) };
        assert_eq!(get_default(&prop), Some(Value::BoolArray(vec![true, false, true])));

        let prop = Property::Symbol {
            nullable: Some(true),
            default: None,
            allowed_values: Some(vec!["red".to_string(), "green".to_string(), "blue".to_string()].into_iter().collect()),
        };
        assert_eq!(get_default(&prop), Some(Value::Null));
    }
}

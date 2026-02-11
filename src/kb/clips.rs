use crate::{
    kb::{KnowledgeBase, KnowledgeBaseError},
    model::{Class, Object, Property, Rule},
};
use clips::{ClipsValue, Environment, Fact, Type};
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
        let env = self.env.lock().map_err(|e| KnowledgeBaseError::KBError(format!("Failed to lock CLIPS environment: {}", e)))?;
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

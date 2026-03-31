use crate::{
    db::Database,
    kb::{KnowledgeBase, KnowledgeBaseEvent},
    model::{Class, CoCoError, CoCoEvent, Object, Rule},
};
use tokio::sync::{broadcast, mpsc, oneshot};
use tracing::{error, info, trace};

pub mod db;
pub mod kb;
pub mod model;
#[cfg(feature = "server")]
pub mod server;

#[derive(Debug)]
pub enum CoCoCommand {
    Init(Vec<Class>, Vec<Rule>, Vec<Object>, oneshot::Sender<Result<(), CoCoError>>),
    GetClasses(oneshot::Sender<Result<Vec<Class>, CoCoError>>),
    GetClass(String, oneshot::Sender<Result<Option<Class>, CoCoError>>),
    CreateClass(Class, oneshot::Sender<Result<(), CoCoError>>),
}

#[derive(Clone)]
pub struct CoCo {
    tx: mpsc::Sender<CoCoCommand>,
    pub event_tx: broadcast::Sender<CoCoEvent>,
}
impl CoCo {
    pub async fn new<DB, KB>(db: DB, mut kb: KB) -> Self
    where
        DB: Database,
        KB: KnowledgeBase,
    {
        let (command_tx, mut command_rx) = mpsc::channel::<CoCoCommand>(100);
        let (event_tx, _) = broadcast::channel(100);

        // Spawn a task to listen for events from the KnowledgeBase and forward them to CoCo's event channel
        let mut event_rx = kb.take_event_receiver().expect("KnowledgeBase must provide an event receiver");
        let event_tx_for_kb = event_tx.clone();
        let event_db = db.clone();
        tokio::spawn(async move {
            while let Some(event) = event_rx.recv().await {
                match event {
                    KnowledgeBaseEvent::AddedClass(object_id, class_name) => match event_db.add_class(object_id.clone(), class_name.clone()).await {
                        Ok(_) => {
                            let _ = event_tx_for_kb.send(CoCoEvent::ClassCreated(class_name));
                        }
                        Err(e) => {
                            error!("Failed to add class to database: {}", e);
                        }
                    },
                    _ => {}
                }
            }
        });

        // Spawn a task to listen for commands from CoCo's command channel and forward them to the KnowledgeBase
        let event_tx_for_commands = event_tx.clone();
        tokio::spawn(async move {
            while let Some(command) = command_rx.recv().await {
                match command {
                    CoCoCommand::Init(classes, rules, objects, response_tx) => {
                        for class in classes {
                            if let Err(e) = kb.create_class(class.clone()).await {
                                let _ = response_tx.send(Err(CoCoError::KnowledgeBaseError(e.to_string())));
                                return;
                            }
                        }
                        for rule in rules {
                            if let Err(e) = kb.create_rule(rule.clone()).await {
                                let _ = response_tx.send(Err(CoCoError::KnowledgeBaseError(e.to_string())));
                                return;
                            }
                        }
                        for object in objects {
                            if let Err(e) = kb.create_object(object.clone()).await {
                                let _ = response_tx.send(Err(CoCoError::KnowledgeBaseError(e.to_string())));
                                return;
                            }
                        }
                        let _ = response_tx.send(Ok(()));
                    }
                    CoCoCommand::GetClasses(response_tx) => {
                        let classes = db.get_classes().await.map_err(|e| CoCoError::DatabaseError(e.to_string()));
                        let _ = response_tx.send(classes);
                    }
                    CoCoCommand::GetClass(class_name, response_tx) => {
                        let class = db.get_class(&class_name).await.map_err(|e| CoCoError::DatabaseError(e.to_string()));
                        let _ = response_tx.send(class);
                    }
                    CoCoCommand::CreateClass(class, response_tx) => {
                        let class_name = class.name.clone();
                        let result = async {
                            kb.create_class(class.clone()).await.map_err(|e| CoCoError::KnowledgeBaseError(e.to_string()))?;
                            db.create_class(class).await.map_err(|e| CoCoError::DatabaseError(e.to_string()))?;
                            Ok::<(), CoCoError>(())
                        }
                        .await;

                        let is_ok = result.is_ok();
                        let _ = response_tx.send(result);
                        if is_ok {
                            let _ = event_tx_for_commands.send(CoCoEvent::ClassCreated(class_name));
                        }
                    }
                }
            }
        });

        CoCo { tx: command_tx, event_tx }
    }

    pub async fn get_classes(&self) -> Result<Vec<Class>, CoCoError> {
        let (response_tx, response_rx) = oneshot::channel();
        self.tx.send(CoCoCommand::GetClasses(response_tx)).await.map_err(|e| CoCoError::KnowledgeBaseError(format!("Failed to send command to CoCo: {}", e)))?;
        response_rx.await.map_err(|e| CoCoError::KnowledgeBaseError(format!("Failed to receive response from CoCo: {}", e)))?
    }

    pub async fn get_class(&self, name: &str) -> Result<Option<Class>, CoCoError> {
        let (response_tx, response_rx) = oneshot::channel();
        self.tx.send(CoCoCommand::GetClass(name.to_owned(), response_tx)).await.map_err(|e| CoCoError::KnowledgeBaseError(format!("Failed to send command to CoCo: {}", e)))?;
        response_rx.await.map_err(|e| CoCoError::KnowledgeBaseError(format!("Failed to receive response from CoCo: {}", e)))?
    }

    pub async fn create_class(&self, class: Class) -> Result<(), CoCoError> {
        let (response_tx, response_rx) = oneshot::channel();
        let class_name = class.name.clone();
        self.tx.send(CoCoCommand::CreateClass(class, response_tx)).await.map_err(|e| CoCoError::KnowledgeBaseError(format!("Failed to send command to CoCo: {}", e)))?;
        let _ = response_rx.await.map_err(|e| CoCoError::KnowledgeBaseError(format!("Failed to receive response from CoCo: {}", e)))?;
        self.event_tx.send(CoCoEvent::ClassCreated(class_name)).map_err(|e| CoCoError::KnowledgeBaseError(format!("Failed to send event from CoCo: {}", e)))?;
        Ok(())
    }
}

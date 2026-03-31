use crate::{
    db::Database,
    kb::{KBCommand, KnowledgeBase, KnowledgeBaseEvent},
    model::{Class, CoCoError, CoCoEvent},
};
use tokio::sync::{broadcast, mpsc, oneshot};

pub mod db;
pub mod kb;
pub mod model;
#[cfg(feature = "server")]
pub mod server;

#[derive(Debug)]
pub enum CoCoCommand {
    GetClasses(oneshot::Sender<Result<Vec<Class>, CoCoError>>),
    GetClass(String, oneshot::Sender<Result<Class, CoCoError>>),
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
        tokio::spawn(async move {
            while let Some(event) = event_rx.recv().await {
                match event {
                    KnowledgeBaseEvent::AddedClass(object_id, class_name) => {
                        let _ = event_tx_for_kb.send(CoCoEvent::AddedClass(object_id, class_name));
                    }
                    _ => {}
                }
            }
        });

        // Spawn a task to listen for commands from CoCo's command channel and forward them to the KnowledgeBase
        tokio::spawn(async move {
            while let Some(command) = command_rx.recv().await {
                match command {
                    CoCoCommand::GetClasses(response_tx) => {
                        let (kb_response_tx, kb_response_rx) = oneshot::channel();
                        let result = async {
                            kb.send_command(KBCommand::GetClasses(kb_response_tx)).await.map_err(|e| CoCoError::KnowledgeBaseError(e.to_string()))?;
                            kb_response_rx.await.map_err(|e| CoCoError::KnowledgeBaseError(format!("Failed to receive response from KnowledgeBase: {}", e)))?.map_err(|e| CoCoError::KnowledgeBaseError(e.to_string()))
                        }
                        .await;
                        let _ = response_tx.send(result);
                    }
                    CoCoCommand::GetClass(class_name, response_tx) => {
                        let (kb_response_tx, kb_response_rx) = oneshot::channel();
                        let result = async {
                            kb.send_command(KBCommand::GetClass(class_name, kb_response_tx)).await.map_err(|e| CoCoError::KnowledgeBaseError(e.to_string()))?;
                            kb_response_rx.await.map_err(|e| CoCoError::KnowledgeBaseError(format!("Failed to receive response from KnowledgeBase: {}", e)))?.map_err(|e| CoCoError::KnowledgeBaseError(e.to_string()))
                        }
                        .await;
                        let _ = response_tx.send(result);
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
}

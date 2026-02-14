use crate::{
    CoCo,
    model::{CoCoEvent, Value},
};
use chrono::{DateTime, Utc};
use rumqttc::v5::{
    AsyncClient, Event, MqttOptions,
    mqttbytes::{QoS, v5::Packet},
};
use std::{collections::HashMap, sync::Arc, time::Duration};

pub fn start_mqtt(coco: Arc<CoCo>, mqtt_broker: &str, mqtt_port: u16) {
    let mut mqttoptions = MqttOptions::new("coco-client-id", mqtt_broker, mqtt_port);
    mqttoptions.set_keep_alive(Duration::from_secs(5));
    let (client, mut eventloop) = AsyncClient::new(mqttoptions, 10);

    let mut rx = coco.get_event_sender().subscribe();
    let coco_clone = coco.clone();
    tokio::spawn(async move {
        while let Ok(event) = rx.recv().await {
            match event {
                CoCoEvent::ClassCreated(class_name) => {
                    let mut update_msg = serde_json::to_value(coco_clone.get_class(&class_name).await).unwrap();
                    update_msg["msg_type"] = serde_json::json!("class_created");
                    let payload = serde_json::to_string(&update_msg).unwrap();
                    client.publish("coco/events", QoS::AtLeastOnce, false, payload).await.unwrap();
                }
                CoCoEvent::ObjectCreated(object_id) => {
                    let mut update_msg = serde_json::to_value(coco_clone.get_object(&object_id).await).unwrap();
                    update_msg["msg_type"] = serde_json::json!("object_created");
                    let payload = serde_json::to_string(&update_msg).unwrap();
                    client.publish("coco/events", QoS::AtLeastOnce, false, payload).await.unwrap();
                    client.subscribe(format!("coco/{}/#", object_id), QoS::AtLeastOnce).await.unwrap();
                }
                CoCoEvent::AddedClass(object_id, class_name) => {
                    let update_msg = serde_json::json!({
                        "msg_type": "added_class",
                        "object_id": object_id,
                        "class_name": class_name
                    });
                    let payload = serde_json::to_string(&update_msg).unwrap();
                    client.publish("coco/events", QoS::AtLeastOnce, false, payload).await.unwrap();
                }
                CoCoEvent::UpdatedProperties(object_id, properties) => {
                    let update_msg = serde_json::json!(properties);
                    let payload = serde_json::to_string(&update_msg).unwrap();
                    client.publish(format!("coco/{}/static", object_id), QoS::AtLeastOnce, false, payload).await.unwrap();
                }
                CoCoEvent::AddedValues(object_id, values, date_time) => {
                    let update_msg = serde_json::json!({
                        "values": values,
                        "date_time": date_time
                    });
                    let payload = serde_json::to_string(&update_msg).unwrap();
                    client.publish(format!("coco/{}/dynamic", object_id), QoS::AtLeastOnce, false, payload).await.unwrap();
                }
                CoCoEvent::RuleCreated(rule) => {
                    let mut update_msg = serde_json::to_value(coco_clone.get_rule(&rule).await.unwrap()).unwrap();
                    update_msg["msg_type"] = serde_json::json!("rule_created");
                    let payload = serde_json::to_string(&update_msg).unwrap();
                    client.publish("coco/events", QoS::AtLeastOnce, false, payload).await.unwrap();
                }
                _ => {}
            }
        }
    });

    tokio::spawn(async move {
        loop {
            match eventloop.poll().await {
                Ok(notification) => {
                    if let Event::Incoming(Packet::Publish(publish)) = notification {
                        let topic = String::from_utf8_lossy(&publish.topic).to_string();
                        let topic_parts: Vec<&str> = topic.split('/').collect();
                        let msg = String::from_utf8_lossy(&publish.payload).to_string();
                        if topic_parts[2] == "static" {
                            let data: HashMap<String, Value> = serde_json::from_str(&msg).unwrap();
                            coco.set_properties(topic_parts[1], data).await.unwrap();
                        } else if topic_parts[2] == "dynamic" {
                            let mut update: serde_json::Value = serde_json::from_str(&msg).unwrap();
                            let values: HashMap<String, Value> = serde_json::from_value(update["values"].take()).unwrap();
                            let date_time: DateTime<Utc> = if update.get("date_time").is_some() { serde_json::from_value(update["date_time"].take()).unwrap() } else { Utc::now() };
                            coco.add_data(topic_parts[1], values, date_time).await.unwrap();
                        }
                    }
                }
                Err(e) => eprintln!("Error: {:?}", e),
            }
        }
    });
}

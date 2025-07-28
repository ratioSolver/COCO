package it.cnr.istc.coco;

import org.apache.hc.client5.http.classic.methods.HttpPost;
import org.apache.hc.client5.http.impl.classic.CloseableHttpClient;
import org.apache.hc.client5.http.impl.classic.HttpClients;
import org.apache.hc.core5.http.io.entity.EntityUtils;
import org.apache.hc.core5.http.io.entity.StringEntity;
import org.keycloak.events.Event;
import org.keycloak.events.EventListenerProvider;
import org.keycloak.events.EventType;
import org.keycloak.events.admin.AdminEvent;

public class UserRegistrationListener implements EventListenerProvider {

    private static final String WEBHOOK_URL = "http://localhost:8080";

    @Override
    public void close() {
    }

    @Override
    public void onEvent(Event event) {
        if (event.getType().equals(EventType.REGISTER)) {
            System.out.println("User registered: " + event.getUserId());
            try (CloseableHttpClient client = HttpClients.createDefault()) {
                HttpPost post = new HttpPost(WEBHOOK_URL + "/items");
                post.setEntity(new StringEntity("{\"type\":\"User\"}"));
                client.execute(post, response -> {
                    if (response.getCode() != 201)
                        throw new RuntimeException("Failed to create user. Response code: " + response.getCode());
                    String user_id = EntityUtils.toString(response.getEntity());
                    System.out.println("User created with ID: " + user_id);
                    return null;
                });
            } catch (Exception e) {
                e.printStackTrace();
            }
        }
    }

    @Override
    public void onEvent(AdminEvent event, boolean includeRepresentation) {
    }
}

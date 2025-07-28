package it.cnr.istc.coco;

import org.keycloak.Config.Scope;
import org.keycloak.events.EventListenerProvider;
import org.keycloak.events.EventListenerProviderFactory;
import org.keycloak.models.KeycloakSession;
import org.keycloak.models.KeycloakSessionFactory;

public class UserRegistrationListenerFactory implements EventListenerProviderFactory {

    @Override
    public void close() {
    }

    @Override
    public EventListenerProvider create(KeycloakSession arg0) {
        return new UserRegistrationListener();
    }

    @Override
    public String getId() {
        return "user-registration-listener";
    }

    @Override
    public void init(Scope config) {
    }

    @Override
    public void postInit(KeycloakSessionFactory arg0) {
    }
}

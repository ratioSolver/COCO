package it.cnr.coco.api;

import java.util.Map;

public interface PropertyType {

    String getName();

    Property createProperty(Map<String, Object> property);
}

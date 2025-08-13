package it.cnr.coco.api;

import com.google.gson.JsonElement;

public record JSONProperty(JsonElement schema, JsonElement defaultValue) implements Property {
}

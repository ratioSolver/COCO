package it.cnr.coco.api;

public record StringProperty(boolean multiple, String defaultValue) implements Property {
}

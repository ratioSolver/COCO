package it.cnr.coco.api;

public record IntProperty(boolean multiple, long min, long max, Long defaultValue) implements Property {
}

package it.cnr.coco.api;

public record FloatProperty(boolean multiple, double min, double max, Double defaultValue) implements Property {
}

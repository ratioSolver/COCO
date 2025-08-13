package it.cnr.coco.api;

public record SymbolProperty(boolean multiple, String[] values, String[] defaultValue) implements Property {
}

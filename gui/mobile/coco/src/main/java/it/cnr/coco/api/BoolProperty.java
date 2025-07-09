package it.cnr.coco.api;

public class BoolProperty implements Property {

    private final Boolean defaultValue;

    public BoolProperty(Boolean defaultValue) {
        this.defaultValue = defaultValue;
    }

    public Boolean getDefaultValue() {
        return defaultValue;
    }
}

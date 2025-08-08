package it.cnr.coco.api;

public class StringProperty implements Property {

    private final boolean multiple;
    private final String defaultValue;

    public StringProperty(boolean multiple, String defaultValue) {
        this.multiple = multiple;
        this.defaultValue = defaultValue;
    }

    public boolean isMultiple() {
        return multiple;
    }

    public String getDefaultValue() {
        return defaultValue;
    }
}

package it.cnr.coco.api;

public class SymbolProperty implements Property {

    private final boolean multiple;
    private final String[] values;
    private final String[] defaultValue;

    public SymbolProperty(boolean multiple, String[] values, String[] defaultValue) {
        this.defaultValue = defaultValue;
        this.multiple = multiple;
        this.values = values;
    }

    public boolean isMultiple() {
        return multiple;
    }

    public String[] getValues() {
        return values;
    }

    public String[] getDefaultValue() {
        return defaultValue;
    }
}

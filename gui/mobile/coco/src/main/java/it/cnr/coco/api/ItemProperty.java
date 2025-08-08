package it.cnr.coco.api;

import androidx.annotation.NonNull;

public class ItemProperty implements Property {

    private final boolean multiple;
    private final Type domain;
    private final Item defaultValue;

    public ItemProperty(boolean multiple, @NonNull Type domain, Item defaultValue) {
        this.multiple = multiple;
        this.domain = domain;
        this.defaultValue = defaultValue;
    }

    public boolean isMultiple() {
        return multiple;
    }

    public Type getDomain() {
        return domain;
    }

    public Item getDefaultValue() {
        return defaultValue;
    }
}

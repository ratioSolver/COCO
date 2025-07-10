package it.cnr.coco.api;

import androidx.annotation.NonNull;

public class ItemProperty implements Property {

    private final Type domain;
    private final Item defaultValue;

    public ItemProperty(@NonNull Type domain, Item defaultValue) {
        this.domain = domain;
        this.defaultValue = defaultValue;
    }

    public Type getDomain() {
        return domain;
    }

    public Item getDefaultValue() {
        return defaultValue;
    }
}

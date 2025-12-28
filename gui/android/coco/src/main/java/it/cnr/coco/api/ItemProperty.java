package it.cnr.coco.api;

import androidx.annotation.NonNull;

public record ItemProperty(boolean multiple, Type domain, Item defaultValue) implements Property {

    public ItemProperty(boolean multiple, @NonNull Type domain, Item defaultValue) {
        this.multiple = multiple;
        this.domain = domain;
        this.defaultValue = defaultValue;
    }
}

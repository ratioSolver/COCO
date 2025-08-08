package it.cnr.coco.api;

import androidx.annotation.NonNull;

import java.util.Map;

class SymbolPropertyType implements PropertyType {

    public SymbolPropertyType() {
    }

    @Override
    public String getName() {
        return "symbol";
    }

    @Override
    public Property createProperty(@NonNull CoCo coco, @NonNull Map<String, Object> property) {
        String[] defaultValue = (String[]) property.get("default");
        boolean multiple = (Boolean) property.getOrDefault("multiple", false);
        String[] values = (String[]) property.get("values");
        return new SymbolProperty(multiple, values, defaultValue);
    }
}
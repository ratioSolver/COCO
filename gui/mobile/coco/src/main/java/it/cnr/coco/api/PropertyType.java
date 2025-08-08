package it.cnr.coco.api;

import androidx.annotation.NonNull;

import java.util.Map;

public interface PropertyType {

    String getName();

    Property createProperty(@NonNull CoCo coco, @NonNull Map<String, Object> property);
}

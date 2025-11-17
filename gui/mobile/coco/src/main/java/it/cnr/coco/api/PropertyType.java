package it.cnr.coco.api;

import androidx.annotation.NonNull;

import com.google.gson.JsonElement;

public interface PropertyType {

    String getName();

    Property createProperty(@NonNull CoCo coco, @NonNull JsonElement property);
}

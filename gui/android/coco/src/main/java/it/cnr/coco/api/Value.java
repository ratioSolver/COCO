package it.cnr.coco.api;

import com.google.gson.JsonElement;
import java.time.Instant;

public record Value(JsonElement data, Instant timestamp) {
}

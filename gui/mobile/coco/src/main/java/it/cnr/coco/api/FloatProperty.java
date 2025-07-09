package it.cnr.coco.api;

public class FloatProperty implements Property {

    private final Double defaultValue;
    private final Double min, max;

    public FloatProperty(Double defaultValue, Double min, Double max) {
        this.defaultValue = defaultValue;
        this.min = min;
        this.max = max;
    }

    public Double getDefaultValue() {
        return defaultValue;
    }

    public Double getMin() {
        return min;
    }

    public Double getMax() {
        return max;
    }
}

package it.cnr.coco.api;

public class FloatProperty implements Property {

    private final boolean multiple;
    private final double min, max;
    private final Double defaultValue;

    public FloatProperty(boolean multiple, double min, double max, Double defaultValue) {
        this.multiple = multiple;
        this.min = min;
        this.max = max;
        this.defaultValue = defaultValue;
    }

    public boolean isMultiple() {
        return multiple;
    }

    public double getMin() {
        return min;
    }

    public double getMax() {
        return max;
    }

    public Double getDefaultValue() {
        return defaultValue;
    }
}

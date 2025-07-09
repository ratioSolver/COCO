package it.cnr.coco.api;

public class IntProperty implements Property {

    private final Long defaultValue;
    private final Long min, max;

    public IntProperty(Long defaultValue, Long min, Long max) {
        this.defaultValue = defaultValue;
        this.min = min;
        this.max = max;
    }

    public Long getDefaultValue() {
        return defaultValue;
    }

    public Long getMin() {
        return min;
    }

    public Long getMax() {
        return max;
    }
}

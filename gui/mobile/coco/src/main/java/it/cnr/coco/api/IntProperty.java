package it.cnr.coco.api;

public class IntProperty implements Property {

    private final boolean multiple;
    private final long min, max;
    private final Long defaultValue;

    public IntProperty(boolean multiple, long min, long max, Long defaultValue) {
        this.multiple = multiple;
        this.min = min;
        this.max = max;
        this.defaultValue = defaultValue;
    }

    public boolean isMultiple() {
        return multiple;
    }

    public long getMin() {
        return min;
    }

    public long getMax() {
        return max;
    }

    public Long getDefaultValue() {
        return defaultValue;
    }
}

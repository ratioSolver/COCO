package it.cnr.coco.api;

public class IntProperty implements Property {

    private Long min, max;

    public IntProperty() {
    }

    public IntProperty(Long min, Long max) {
        this.min = min;
        this.max = max;
    }

    public Long getMin() {
        return min;
    }

    public void setMin(Long min) {
        this.min = min;
    }

    public Long getMax() {
        return max;
    }

    public void setMax(Long max) {
        this.max = max;
    }
}

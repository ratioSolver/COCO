package it.cnr.coco.model;

public class FloatProperty implements Property {

    private Double min, max;

    public FloatProperty() {
    }

    public FloatProperty(Double min, Double max) {
        this.min = min;
        this.max = max;
    }

    public Double getMin() {
        return min;
    }

    public void setMin(Double min) {
        this.min = min;
    }

    public Double getMax() {
        return max;
    }

    public void setMax(Double max) {
        this.max = max;
    }
}

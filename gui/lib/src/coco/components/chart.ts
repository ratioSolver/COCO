import { coco } from '../coco';
import { Data } from 'plotly.js-dist-min';

export namespace chart {

  export class ChartManager {

    private static instance: ChartManager;
    private charts: Map<string, ChartGenerator<unknown>> = new Map();

    private constructor() {
      this.add_chart_generator(new BoolChartGenerator());
    }

    public static get_instance(): ChartManager {
      if (!ChartManager.instance)
        ChartManager.instance = new ChartManager();
      return ChartManager.instance;
    }

    add_chart_generator<V>(chart: ChartGenerator<V>) { this.charts.set(chart.get_name(), chart); }
    get_chart_generator(name: string): ChartGenerator<unknown> { return this.charts.get(name)!; }
  }

  export abstract class ChartGenerator<V> {

    private name: string;

    constructor(name: string) {
      this.name = name;
    }

    get_name(): string { return this.name; }

    abstract make_chart(name: string, property: coco.taxonomy.Property<unknown>, vals: Value<V>[]): Chart<V>;
  }

  export type Value<V> = { value: V, timestamp: Date };

  class BoolChartGenerator extends ChartGenerator<boolean> {

    constructor() { super('bool'); }

    make_chart(name: string, property: coco.taxonomy.BoolProperty, vals: Value<boolean>[]): BoolChart { return new BoolChart(name, property, vals); }
  }

  export interface Chart<V> {

    set_values(vals: { value: V, timestamp: Date }[]): void;
  }

  class BoolChart implements Chart<boolean> {

    private readonly name: string;
    private readonly colors: Map<string, string> = new Map();
    private readonly data: Data[] = [];

    constructor(name: string, _: coco.taxonomy.BoolProperty, vals: Value<boolean>[]) {
      this.name = name;
      this.colors.set('true', '#00ff00');
      this.colors.set('false', '#ff0000');
      this.set_values(vals);
    }

    set_values(vals: Value<boolean>[]): void {
      this.data.length = 0;
      for (let i = 0; i < vals.length - 1; i++)
        this.data.push({ x: [vals[i].timestamp, vals[i + 1].timestamp], y: [1, 1], name: vals[i] ? 'True' : 'False', type: 'scatter', opacity: 0.7, mode: 'lines', line: { width: 30, color: vals[i] ? this.colors.get('true') : this.colors.get('false') }, yaxis: this.name });
    }
  }
}
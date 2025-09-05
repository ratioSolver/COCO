import { coco } from '../coco';
import { PlotData } from 'plotly.js-dist-min';
import { scaleOrdinal } from "d3-scale";
import { schemeCategory10 } from "d3-scale-chromatic";

export namespace chart {

  export class ChartManager {

    private static instance: ChartManager;
    private charts: Map<string, ChartGenerator<unknown>> = new Map();

    private constructor() {
      this.add_chart_generator(new BoolChartGenerator());
      this.add_chart_generator(new IntChartGenerator());
      this.add_chart_generator(new FloatChartGenerator());
      this.add_chart_generator(new StringChartGenerator());
      this.add_chart_generator(new SymbolChartGenerator());
      this.add_chart_generator(new ItemChartGenerator());
      this.add_chart_generator(new JSONChartGenerator());
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

    show_tick_labels(): boolean | undefined { return undefined; }
    show_grid(): boolean | undefined { return undefined; }

    abstract make_chart(prop: coco.taxonomy.Property<unknown>, vals: Value<V>[]): Chart<V>;
  }

  export type Value<V> = { value: V, timestamp: Date };

  class BoolChartGenerator extends ChartGenerator<boolean> {

    constructor() { super('bool'); }

    make_chart(prop: coco.taxonomy.BoolProperty, vals: Value<boolean>[]): BoolChart { return new BoolChart(prop, vals); }

    override show_tick_labels(): boolean { return false; }
    override show_grid(): boolean { return false; }
  }

  class IntChartGenerator extends ChartGenerator<number> {

    constructor() { super('int'); }

    make_chart(prop: coco.taxonomy.IntProperty, vals: Value<number>[]): IntChart { return new IntChart(prop, vals); }
  }

  class FloatChartGenerator extends ChartGenerator<number> {

    constructor() { super('float'); }

    make_chart(prop: coco.taxonomy.FloatProperty, vals: Value<number>[]): FloatChart { return new FloatChart(prop, vals); }
  }

  class StringChartGenerator extends ChartGenerator<string> {

    constructor() { super('string'); }

    make_chart(prop: coco.taxonomy.StringProperty, vals: Value<string>[]): StringChart { return new StringChart(prop, vals); }

    override show_tick_labels(): boolean { return false; }
    override show_grid(): boolean { return false; }
  }

  class SymbolChartGenerator extends ChartGenerator<string | string[]> {

    constructor() { super('symbol'); }

    make_chart(prop: coco.taxonomy.SymbolProperty, vals: Value<string | string[]>[]): SymbolChart { return new SymbolChart(prop, vals); }

    override show_tick_labels(): boolean { return false; }
    override show_grid(): boolean { return false; }
  }

  class ItemChartGenerator extends ChartGenerator<string | string[]> {

    constructor() { super('item'); }

    make_chart(prop: coco.taxonomy.ItemProperty, vals: Value<string | string[]>[]): ItemChart { return new ItemChart(prop, vals); }

    override show_tick_labels(): boolean { return false; }
    override show_grid(): boolean { return false; }
  }

  class JSONChartGenerator extends ChartGenerator<Record<string, any>> {

    constructor() { super('json'); }

    make_chart(prop: coco.taxonomy.JSONProperty, vals: Value<Record<string, any>>[]): JSONChart { return new JSONChart(prop, vals); }

    override show_tick_labels(): boolean { return false; }
    override show_grid(): boolean { return false; }
  }

  export interface Chart<V> {

    set_data(vals: Value<V>[]): void;
    set_datum(val: Value<V>): void;
    extend(timestamp: Date): void;
    get_data(): Partial<PlotData>[];

    get_range(): number[] | undefined;
  }

  class BoolChart implements Chart<boolean> {

    private readonly data: Partial<PlotData>[] = [];
    private readonly colors = scaleOrdinal<boolean, string>().domain([true, false]).range(["#4CAF50", "#F44336"]);

    constructor(_: coco.taxonomy.BoolProperty, vals: Value<boolean>[]) {
      this.set_data(vals);
    }

    set_data(vals: Value<boolean>[]): void {
      this.data.length = 0;
      for (let i = 0; i < vals.length - 1; i++)
        this.data.push({ x: [vals[i].timestamp.valueOf(), vals[i + 1].timestamp.valueOf()], y: [1, 1], name: vals[i].value ? 'true' : 'false', type: 'scatter', opacity: 0.7, mode: 'lines', line: { width: 30, color: this.colors(vals[i].value) } });
      if (vals.length)
        this.data.push({ x: [vals[vals.length - 1].timestamp.valueOf(), vals[vals.length - 1].timestamp.valueOf() + 1], y: [1, 1], name: vals[vals.length - 1].value ? 'true' : 'false', type: 'scatter', opacity: 0.7, mode: 'lines', line: { width: 30, color: this.colors(vals[vals.length - 1].value) } });
    }
    set_datum(val: Value<boolean>): void {
      if (this.data.length)
        this.data[this.data.length - 1].x![1] = val.timestamp.valueOf();
      this.data.push({ x: [val.timestamp.valueOf(), val.timestamp.valueOf() + 1], y: [1, 1], name: val.value ? 'true' : 'false', type: 'scatter', opacity: 0.7, mode: 'lines', line: { width: 30, color: this.colors(val.value) } });
    }
    extend(timestamp: Date): void {
      if (this.data.length) // Update the last data point's end time to the current timestamp
        this.data[this.data.length - 1].x![1] = timestamp.valueOf();
    }

    get_data(): Partial<PlotData>[] { return this.data; }

    get_range(): undefined { return undefined; }
  }

  class IntChart implements Chart<number> {

    private readonly prop: coco.taxonomy.IntProperty;
    private readonly data: Partial<PlotData>;

    constructor(prop: coco.taxonomy.IntProperty, vals: Value<number>[]) {
      this.prop = prop;
      this.data = { x: [], y: [], type: 'scatter' };
      this.set_data(vals);
    }

    set_data(vals: Value<number>[]): void {
      for (let i = 0; i < vals.length - 1; i++) {
        (this.data.x! as number[]).push(vals[i].timestamp.valueOf());
        (this.data.y! as number[]).push(vals[i].value);
      }
    }
    set_datum(val: Value<number>): void {
      this.data.x = [...this.data.x! as number[], val.timestamp.valueOf()];
      this.data.y = [...this.data.y! as number[], val.value];
    }
    extend(timestamp: Date): void {
      if (this.data.x && this.data.y) { // Update the last data point's end time to the current timestamp
        this.data.x = [...this.data.x as number[], timestamp.valueOf()];
        this.data.y = [...this.data.y as number[], this.data.y[this.data.y.length - 1] as number];
      }
    }

    get_data(): Partial<PlotData>[] { return [this.data]; }

    get_range(): number[] | undefined {
      const min = this.prop.get_min();
      const max = this.prop.get_max();
      return min && max ? [min, max] : undefined;
    }
  }

  class FloatChart implements Chart<number> {

    private readonly prop: coco.taxonomy.FloatProperty;
    private readonly data: Partial<PlotData>;

    constructor(prop: coco.taxonomy.FloatProperty, vals: Value<number>[]) {
      this.prop = prop;
      this.data = { x: [], y: [], type: 'scatter' };
      this.set_data(vals);
    }

    set_data(vals: Value<number>[]): void {
      for (let i = 0; i < vals.length - 1; i++) {
        (this.data.x! as number[]).push(vals[i].timestamp.valueOf());
        (this.data.y! as number[]).push(vals[i].value);
      }
    }
    set_datum(val: Value<number>): void {
      (this.data.x! as number[]).push(val.timestamp.valueOf());
      (this.data.y! as number[]).push(val.value);
    }
    extend(timestamp: Date): void {
      if (this.data.x && this.data.y) { // Update the last data point's end time to the current timestamp
        (this.data.x as number[]).push(timestamp.valueOf());
        (this.data.y as number[]).push(this.data.y![this.data.y!.length - 1] as number);
      }
    }

    get_data(): Partial<PlotData>[] { return [this.data]; }

    get_range(): number[] | undefined {
      const min = this.prop.get_min();
      const max = this.prop.get_max();
      return min && max ? [min, max] : undefined;
    }
  }

  class StringChart implements Chart<string | null> {

    private readonly data: Partial<PlotData>[] = [];
    private readonly colors = scaleOrdinal(schemeCategory10);

    constructor(_: coco.taxonomy.StringProperty, vals: Value<string | null>[]) {
      this.set_data(vals);
    }

    set_data(vals: Value<string | null>[]): void {
      this.data.length = 0;
      for (let i = 0; i < vals.length - 1; i++)
        if (vals[i].value)
          this.data.push({ x: [vals[i].timestamp.valueOf(), vals[i + 1].timestamp.valueOf()], y: [1, 1], name: vals[i].value!, type: 'scatter', opacity: 0.7, mode: 'lines', line: { width: 30, color: this.colors(vals[i].value!) } });
      if (vals.length && vals[vals.length - 1].value)
        this.data.push({ x: [vals[vals.length - 1].timestamp.valueOf(), vals[vals.length - 1].timestamp.valueOf() + 1], y: [1, 1], name: vals[vals.length - 1].value!, type: 'scatter', opacity: 0.7, mode: 'lines', line: { width: 30, color: this.colors(vals[vals.length - 1].value!) } });
    }
    set_datum(val: Value<string | null>): void {
      if (this.data.length) // Update the last data point's end time to the current timestamp
        this.data[this.data.length - 1].x![1] = val.timestamp.valueOf();
      if (val.value)
        this.data.push({ x: [val.timestamp.valueOf(), val.timestamp.valueOf() + 1], y: [1, 1], name: val.value, type: 'scatter', opacity: 0.7, mode: 'lines', line: { width: 30, color: this.colors(val.value) } });
    }
    extend(timestamp: Date): void {
      if (this.data.length) // Update the last data point's end time to the current timestamp
        this.data[this.data.length - 1].x![1] = timestamp.valueOf();
    }

    get_data(): Partial<PlotData>[] { return this.data; }

    get_range(): undefined { return undefined; }
  }

  class SymbolChart implements Chart<string | string[] | null> {

    private readonly data: Partial<PlotData>[] = [];
    private readonly colors = scaleOrdinal(schemeCategory10);

    constructor(_: coco.taxonomy.SymbolProperty, vals: Value<string | string[] | null>[]) {
      this.set_data(vals);
    }

    set_data(vals: Value<string | string[] | null>[]): void {
      this.data.length = 0;
      for (let i = 0; i < vals.length - 1; i++)
        if (vals[i].value)
          this.data.push({ x: [vals[i].timestamp.valueOf(), vals[i + 1].timestamp.valueOf()], y: [1, 1], name: this.get_name(vals[i].value!), type: 'scatter', opacity: 0.7, mode: 'lines', line: { width: 30, color: this.colors(this.get_name(vals[i].value!)) } });
      if (vals.length && vals[vals.length - 1].value)
        this.data.push({ x: [vals[vals.length - 1].timestamp.valueOf(), vals[vals.length - 1].timestamp.valueOf() + 1], y: [1, 1], name: this.get_name(vals[vals.length - 1].value!), type: 'scatter', opacity: 0.7, mode: 'lines', line: { width: 30, color: this.colors(this.get_name(vals[vals.length - 1].value!)) } });
    }
    set_datum(val: Value<string | string[] | null>): void {
      if (this.data.length) // Update the last data point's end time to the current timestamp
        this.data[this.data.length - 1].x![1] = val.timestamp.valueOf();
      if (val.value)
        this.data.push({ x: [val.timestamp.valueOf(), val.timestamp.valueOf() + 1], y: [1, 1], name: this.get_name(val.value), type: 'scatter', opacity: 0.7, mode: 'lines', line: { width: 30, color: this.colors(this.get_name(val.value)) } });
    }
    extend(timestamp: Date): void {
      if (this.data.length) // Update the last data point's end time to the current timestamp
        this.data[this.data.length - 1].x![1] = timestamp.valueOf();
    }

    get_data(): Partial<PlotData>[] { return this.data; }

    get_range(): undefined { return undefined; }

    private get_name(val: string | string[]): string { return Array.isArray(val) ? (val as string[]).join(', ') : val as string; }
  }

  class ItemChart implements Chart<string | string[] | null> {

    private readonly data: Partial<PlotData>[] = [];
    private readonly colors = scaleOrdinal(schemeCategory10);

    constructor(_: coco.taxonomy.ItemProperty, vals: Value<string | string[] | null>[]) {
      this.set_data(vals);
    }

    set_data(vals: Value<string | string[] | null>[]): void {
      this.data.length = 0;
      for (let i = 0; i < vals.length - 1; i++)
        if (vals[i].value)
          this.data.push({ x: [vals[i].timestamp.valueOf(), vals[i + 1].timestamp.valueOf()], y: [1, 1], name: this.get_name(vals[i].value!), type: 'scatter', opacity: 0.7, mode: 'lines', line: { width: 30, color: this.colors(this.get_name(vals[i].value!)) } });
      if (vals.length && vals[vals.length - 1].value)
        this.data.push({ x: [vals[vals.length - 1].timestamp.valueOf(), vals[vals.length - 1].timestamp.valueOf() + 1], y: [1, 1], name: this.get_name(vals[vals.length - 1].value!), type: 'scatter', opacity: 0.7, mode: 'lines', line: { width: 30, color: this.colors(this.get_name(vals[vals.length - 1].value!)) } });
    }
    set_datum(val: Value<string | string[] | null>): void {
      if (this.data.length) // Update the last data point's end time to the current timestamp
        this.data[this.data.length - 1].x![1] = val.timestamp.valueOf();
      if (val.value)
        this.data.push({ x: [val.timestamp.valueOf(), val.timestamp.valueOf() + 1], y: [1, 1], name: this.get_name(val.value), type: 'scatter', opacity: 0.7, mode: 'lines', line: { width: 30, color: this.colors(this.get_name(val.value)) } });
    }
    extend(timestamp: Date): void {
      if (this.data.length) // Update the last data point's end time to the current timestamp
        this.data[this.data.length - 1].x![1] = timestamp.valueOf();
    }

    get_data(): Partial<PlotData>[] { return this.data; }

    get_range(): undefined { return undefined; }

    private get_name(val: string | string[]): string { return Array.isArray(val) ? (val as string[]).join(', ') : val as string; }
  }

  class JSONChart implements Chart<Record<string, any> | null> {

    private readonly data: Partial<PlotData>[] = [];
    private readonly colors = scaleOrdinal(schemeCategory10);

    constructor(_: coco.taxonomy.JSONProperty, vals: Value<Record<string, any> | null>[]) {
      this.set_data(vals);
    }

    set_data(vals: Value<Record<string, any> | null>[]): void {
      this.data.length = 0;
      for (let i = 0; i < vals.length - 1; i++) {
        const name = JSON.stringify(vals[i].value);
        this.data.push({ x: [vals[i].timestamp.valueOf(), vals[i + 1].timestamp.valueOf()], y: [1, 1], name: name, type: 'scatter', opacity: 0.7, mode: 'lines', line: { width: 30, color: this.colors(name) } });
      }
      if (vals.length) {
        const name = JSON.stringify(vals[vals.length - 1].value);
        this.data.push({ x: [vals[vals.length - 1].timestamp.valueOf(), vals[vals.length - 1].timestamp.valueOf() + 1], y: [1, 1], name: name, type: 'scatter', opacity: 0.7, mode: 'lines', line: { width: 30, color: this.colors(name) } });
      }
    }
    set_datum(val: Value<Record<string, any> | null>): void {
      if (this.data.length) // Update the last data point's end time to the current timestamp
        this.data[this.data.length - 1].x![1] = val.timestamp.valueOf();
      const name = JSON.stringify(val.value);
      this.data.push({ x: [val.timestamp.valueOf(), val.timestamp.valueOf() + 1], y: [1, 1], name: name, type: 'scatter', opacity: 0.7, mode: 'lines', line: { width: 30, color: this.colors(name) } });
    }
    extend(timestamp: Date): void {
      if (this.data.length) // Update the last data point's end time to the current timestamp
        this.data[this.data.length - 1].x![1] = timestamp.valueOf();
    }

    get_data(): Partial<PlotData>[] { return this.data; }

    get_range(): undefined { return undefined; }
  }
}
import { coco } from '../coco';
import { Data, PlotData } from 'plotly.js-dist-min';
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

    abstract make_chart(name: string, prop: coco.taxonomy.Property<unknown>, vals: Value<V>[]): Chart<V>;
  }

  export type Value<V> = { value: V, timestamp: Date };

  class BoolChartGenerator extends ChartGenerator<boolean> {

    constructor() { super('bool'); }

    make_chart(name: string, prop: coco.taxonomy.BoolProperty, vals: Value<boolean>[]): BoolChart { return new BoolChart(name, prop, vals); }

    override show_tick_labels(): boolean { return false; }
    override show_grid(): boolean { return false; }
  }

  class IntChartGenerator extends ChartGenerator<number> {

    constructor() { super('int'); }

    make_chart(name: string, prop: coco.taxonomy.IntProperty, vals: Value<number>[]): IntChart { return new IntChart(name, prop, vals); }
  }

  class FloatChartGenerator extends ChartGenerator<number> {

    constructor() { super('float'); }

    make_chart(name: string, prop: coco.taxonomy.FloatProperty, vals: Value<number>[]): FloatChart { return new FloatChart(name, prop, vals); }
  }

  class StringChartGenerator extends ChartGenerator<string> {

    constructor() { super('string'); }

    make_chart(name: string, prop: coco.taxonomy.StringProperty, vals: Value<string>[]): StringChart { return new StringChart(name, prop, vals); }

    override show_tick_labels(): boolean { return false; }
    override show_grid(): boolean { return false; }
  }

  class SymbolChartGenerator extends ChartGenerator<string | string[]> {

    constructor() { super('symbol'); }

    make_chart(name: string, prop: coco.taxonomy.SymbolProperty, vals: Value<string | string[]>[]): SymbolChart { return new SymbolChart(name, prop, vals); }

    override show_tick_labels(): boolean { return false; }
    override show_grid(): boolean { return false; }
  }

  class ItemChartGenerator extends ChartGenerator<string | string[]> {

    constructor() { super('item'); }

    make_chart(name: string, prop: coco.taxonomy.ItemProperty, vals: Value<string | string[]>[]): ItemChart { return new ItemChart(name, prop, vals); }

    override show_tick_labels(): boolean { return false; }
    override show_grid(): boolean { return false; }
  }

  class JSONChartGenerator extends ChartGenerator<Record<string, any>> {

    constructor() { super('item'); }

    make_chart(name: string, prop: coco.taxonomy.JSONProperty, vals: Value<Record<string, any>>[]): JSONChart { return new JSONChart(name, prop, vals); }

    override show_tick_labels(): boolean { return false; }
    override show_grid(): boolean { return false; }
  }

  export interface Chart<V> {

    set_values(vals: Value<V>[]): void;
    add_value(val: Value<V>): Data[];
    get_data(): Data[];

    get_range(): number[] | undefined;
  }

  class BoolChart implements Chart<boolean> {

    private readonly name: string;
    private readonly data: Partial<PlotData>[] = [];
    private readonly colors = scaleOrdinal<boolean, string>().domain([true, false]).range(["#4CAF50", "#F44336"]);

    constructor(name: string, _: coco.taxonomy.BoolProperty, vals: Value<boolean>[]) {
      this.name = name;
      this.set_values(vals);
    }

    set_values(vals: Value<boolean>[]): void {
      this.data.length = 0;
      for (let i = 0; i < vals.length - 1; i++)
        this.data.push({ x: [vals[i].timestamp.valueOf(), vals[i + 1].timestamp.valueOf()], y: [1, 1], type: 'scatter', opacity: 0.7, mode: 'lines', line: { width: 30, color: this.colors(vals[i].value) }, yaxis: 'yaxis-' + this.name });
      if (vals.length)
        this.data.push({ x: [vals[vals.length - 1].timestamp.valueOf(), vals[vals.length - 1].timestamp.valueOf() + 1], y: [1, 1], type: 'scatter', opacity: 0.7, mode: 'lines', line: { width: 30, color: this.colors(vals[vals.length - 1].value) }, yaxis: 'yaxis-' + this.name });
    }
    add_value(val: Value<boolean>): Data[] {
      if (this.data.length)
        this.data[this.data.length - 1].x![1] = val.timestamp.valueOf();
      this.data.push({ x: [val.timestamp.valueOf(), val.timestamp.valueOf() + 1], y: [1, 1], type: 'scatter', opacity: 0.7, mode: 'lines', line: { width: 30, color: this.colors(val.value) }, yaxis: 'yaxis-' + this.name });
      if (this.data.length > 1)
        return [this.data[this.data.length - 2], this.data[this.data.length - 1]];
      else
        return [this.data[this.data.length - 1]];
    }

    get_data(): Data[] { return this.data; }

    get_range(): undefined { return undefined; }
  }

  class IntChart implements Chart<number> {

    private readonly name: string;
    private readonly prop: coco.taxonomy.IntProperty;
    private readonly data: Partial<PlotData>;

    constructor(name: string, prop: coco.taxonomy.IntProperty, vals: Value<number>[]) {
      this.name = name;
      this.prop = prop;
      this.data = { x: [], y: [], type: 'scatter', yaxis: 'yaxis-' + this.name };
      this.set_values(vals);
    }

    set_values(vals: Value<number>[]): void {
      for (let i = 0; i < vals.length - 1; i++) {
        (this.data.x! as number[]).push(vals[i].timestamp.valueOf());
        (this.data.y! as number[]).push(vals[i].value);
      }
    }
    add_value(val: Value<number>): Data[] {
      (this.data.x! as number[]).push(val.timestamp.valueOf());
      (this.data.y! as number[]).push(val.value);
      return [this.data];
    }

    get_data(): Data[] { return [this.data]; }

    get_range(): number[] | undefined {
      const min = this.prop.get_min();
      const max = this.prop.get_max();
      return min && max ? [min, max] : undefined;
    }
  }

  class FloatChart implements Chart<number> {

    private readonly name: string;
    private readonly prop: coco.taxonomy.FloatProperty;
    private readonly data: Partial<PlotData>;

    constructor(name: string, prop: coco.taxonomy.FloatProperty, vals: Value<number>[]) {
      this.name = name;
      this.prop = prop;
      this.data = { x: [], y: [], type: 'scatter', yaxis: 'yaxis-' + this.name };
      this.set_values(vals);
    }

    set_values(vals: Value<number>[]): void {
      for (let i = 0; i < vals.length - 1; i++) {
        (this.data.x! as number[]).push(vals[i].timestamp.valueOf());
        (this.data.y! as number[]).push(vals[i].value);
      }
    }
    add_value(val: Value<number>): Data[] {
      (this.data.x! as number[]).push(val.timestamp.valueOf());
      (this.data.y! as number[]).push(val.value);
      return [this.data];
    }

    get_data(): Data[] { return [this.data]; }

    get_range(): number[] | undefined {
      const min = this.prop.get_min();
      const max = this.prop.get_max();
      return min && max ? [min, max] : undefined;
    }
  }

  class StringChart implements Chart<string> {

    private readonly name: string;
    private readonly data: Partial<PlotData>[] = [];
    private readonly colors = scaleOrdinal(schemeCategory10);

    constructor(name: string, _: coco.taxonomy.StringProperty, vals: Value<string>[]) {
      this.name = name;
      this.set_values(vals);
    }

    set_values(vals: Value<string>[]): void {
      this.data.length = 0;
      for (let i = 0; i < vals.length - 1; i++)
        this.data.push({ x: [vals[i].timestamp.valueOf(), vals[i + 1].timestamp.valueOf()], y: [1, 1], name: vals[i].value, type: 'scatter', opacity: 0.7, mode: 'lines', line: { width: 30, color: this.colors(vals[i].value) }, yaxis: 'yaxis-' + this.name });
      if (vals.length)
        this.data.push({ x: [vals[vals.length - 1].timestamp.valueOf(), vals[vals.length - 1].timestamp.valueOf() + 1], y: [1, 1], type: 'scatter', opacity: 0.7, mode: 'lines', line: { width: 30, color: this.colors(vals[vals.length - 1].value) }, yaxis: 'yaxis-' + this.name });
    }
    add_value(val: Value<string>): Data[] {
      if (this.data.length)
        this.data[this.data.length - 1].x![1] = val.timestamp.valueOf();
      this.data.push({ x: [val.timestamp.valueOf(), val.timestamp.valueOf() + 1], y: [1, 1], type: 'scatter', opacity: 0.7, mode: 'lines', line: { width: 30, color: this.colors(val.value) }, yaxis: 'yaxis-' + this.name });
      if (this.data.length > 1)
        return [this.data[this.data.length - 2], this.data[this.data.length - 1]];
      else
        return [this.data[this.data.length - 1]];
    }

    get_data(): Data[] { return this.data; }

    get_range(): undefined { return undefined; }
  }

  class SymbolChart implements Chart<string | string[]> {

    private readonly name: string;
    private readonly data: Partial<PlotData>[] = [];
    private readonly colors = scaleOrdinal(schemeCategory10);

    constructor(name: string, _: coco.taxonomy.SymbolProperty, vals: Value<string | string[]>[]) {
      this.name = name;
      this.set_values(vals);
    }

    set_values(vals: Value<string | string[]>[]): void {
      this.data.length = 0;
      for (let i = 0; i < vals.length - 1; i++) {
        const name = Array.isArray(vals[i].value) ? (vals[i].value as string[]).join(', ') : vals[i].value as string;
        this.data.push({ x: [vals[i].timestamp.valueOf(), vals[i + 1].timestamp.valueOf()], y: [1, 1], name: name, type: 'scatter', opacity: 0.7, mode: 'lines', line: { width: 30, color: this.colors(name) }, yaxis: 'yaxis-' + this.name });
      }
      if (vals.length) {
        const name = Array.isArray(vals[vals.length - 1].value) ? (vals[vals.length - 1].value as string[]).join(', ') : vals[vals.length - 1].value as string;
        this.data.push({ x: [vals[vals.length - 1].timestamp.valueOf(), vals[vals.length - 1].timestamp.valueOf() + 1], y: [1, 1], name: name, type: 'scatter', opacity: 0.7, mode: 'lines', line: { width: 30, color: this.colors(name) }, yaxis: 'yaxis-' + this.name });
      }
    }
    add_value(val: Value<string | string[]>): Data[] {
      if (this.data.length)
        this.data[this.data.length - 1].x![1] = val.timestamp.valueOf();
      const name = Array.isArray(val.value) ? (val.value as string[]).join(', ') : val.value as string;
      this.data.push({ x: [val.timestamp.valueOf(), val.timestamp.valueOf() + 1], y: [1, 1], name: name, type: 'scatter', opacity: 0.7, mode: 'lines', line: { width: 30, color: this.colors(name) }, yaxis: 'yaxis-' + this.name });
      if (this.data.length > 1)
        return [this.data[this.data.length - 2], this.data[this.data.length - 1]];
      else
        return [this.data[this.data.length - 1]];
    }

    get_data(): Data[] { return this.data; }

    get_range(): undefined { return undefined; }
  }

  class ItemChart implements Chart<string | string[]> {

    private readonly name: string;
    private readonly data: Partial<PlotData>[] = [];
    private readonly colors = scaleOrdinal(schemeCategory10);

    constructor(name: string, _: coco.taxonomy.ItemProperty, vals: Value<string | string[]>[]) {
      this.name = name;
      this.set_values(vals);
    }

    set_values(vals: Value<string | string[]>[]): void {
      this.data.length = 0;
      for (let i = 0; i < vals.length - 1; i++) {
        const name = Array.isArray(vals[i].value) ? (vals[i].value as string[]).join(', ') : vals[i].value as string;
        this.data.push({ x: [vals[i].timestamp.valueOf(), vals[i + 1].timestamp.valueOf()], y: [1, 1], name: name, type: 'scatter', opacity: 0.7, mode: 'lines', line: { width: 30, color: this.colors(name) }, yaxis: 'yaxis-' + this.name });
      }
      if (vals.length) {
        const name = Array.isArray(vals[vals.length - 1].value) ? (vals[vals.length - 1].value as string[]).join(', ') : vals[vals.length - 1].value as string;
        this.data.push({ x: [vals[vals.length - 1].timestamp.valueOf(), vals[vals.length - 1].timestamp.valueOf() + 1], y: [1, 1], name: name, type: 'scatter', opacity: 0.7, mode: 'lines', line: { width: 30, color: this.colors(name) }, yaxis: 'yaxis-' + this.name });
      }
    }
    add_value(val: Value<string | string[]>): Data[] {
      if (this.data.length)
        this.data[this.data.length - 1].x![1] = val.timestamp.valueOf();
      const name = Array.isArray(val.value) ? (val.value as string[]).join(', ') : val.value as string;
      this.data.push({ x: [val.timestamp.valueOf(), val.timestamp.valueOf() + 1], y: [1, 1], name: name, type: 'scatter', opacity: 0.7, mode: 'lines', line: { width: 30, color: this.colors(name) }, yaxis: 'yaxis-' + this.name });
      if (this.data.length > 1)
        return [this.data[this.data.length - 2], this.data[this.data.length - 1]];
      else
        return [this.data[this.data.length - 1]];
    }

    get_data(): Data[] { return this.data; }

    get_range(): undefined { return undefined; }
  }

  class JSONChart implements Chart<Record<string, any>> {

    private readonly name: string;
    private readonly data: Partial<PlotData>[] = [];
    private readonly colors = scaleOrdinal(schemeCategory10);

    constructor(name: string, _: coco.taxonomy.JSONProperty, vals: Value<Record<string, any>>[]) {
      this.name = name;
      this.set_values(vals);
    }

    set_values(vals: Value<Record<string, any>>[]): void {
      this.data.length = 0;
      for (let i = 0; i < vals.length - 1; i++) {
        const name = JSON.stringify(vals[i].value);
        this.data.push({ x: [vals[i].timestamp.valueOf(), vals[i + 1].timestamp.valueOf()], y: [1, 1], name: name, type: 'scatter', opacity: 0.7, mode: 'lines', line: { width: 30, color: this.colors(name) }, yaxis: 'yaxis-' + this.name });
      }
      if (vals.length) {
        const name = JSON.stringify(vals[vals.length - 1].value);
        this.data.push({ x: [vals[vals.length - 1].timestamp.valueOf(), vals[vals.length - 1].timestamp.valueOf() + 1], y: [1, 1], name: name, type: 'scatter', opacity: 0.7, mode: 'lines', line: { width: 30, color: this.colors(name) }, yaxis: 'yaxis-' + this.name });
      }
    }
    add_value(val: Value<Record<string, any>>): Data[] {
      if (this.data.length)
        this.data[this.data.length - 1].x![1] = val.timestamp.valueOf();
      const name = JSON.stringify(val.value);
      this.data.push({ x: [val.timestamp.valueOf(), val.timestamp.valueOf() + 1], y: [1, 1], name: name, type: 'scatter', opacity: 0.7, mode: 'lines', line: { width: 30, color: this.colors(name) }, yaxis: 'yaxis-' + this.name });
      if (this.data.length > 1)
        return [this.data[this.data.length - 2], this.data[this.data.length - 1]];
      else
        return [this.data[this.data.length - 1]];
    }

    get_data(): Data[] { return this.data; }

    get_range(): undefined { return undefined; }
  }
}
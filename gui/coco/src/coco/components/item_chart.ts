import { PayloadComponent } from "@ratiosolver/flick";
import { coco } from "../coco";
import Plotly, { Layout, PlotData } from "plotly.js-dist-min";
import { chart } from "./chart";

export class ItemChart extends PayloadComponent<HTMLDivElement, coco.taxonomy.Item> implements coco.taxonomy.ItemListener {

  private readonly layout: Partial<Layout> & { [key: `yaxis${number}`]: Partial<Plotly.LayoutAxis>; } = { autosize: true, xaxis: { title: { text: 'Time' }, type: 'date' }, showlegend: false };
  private readonly config = { responsive: true, displaylogo: false };
  private readonly charts: Map<string, chart.Chart<unknown>> = new Map();
  private readonly yaxis: Map<string, string> = new Map();

  constructor(item: coco.taxonomy.Item) {
    super(document.createElement('div'), item);
    this.node.style.width = '100%';
    this.node.style.height = coco.taxonomy.get_dynamic_properties(this.payload).size * 200 + 'px';

    item.add_item_listener(this);
  }

  override mounted(): void {
    const dynamic_props = coco.taxonomy.get_dynamic_properties(this.payload);
    const values: Map<string, chart.Value<unknown>[]> = new Map();
    for (const val of this.payload.get_data())
      for (const [name, v] of Object.entries(val.data)) {
        if (!values.has(name))
          values.set(name, []);
        values.get(name)!.push({ value: v, timestamp: val.timestamp });
      }

    const data: Partial<PlotData>[] = [];
    let i = dynamic_props.size;
    const domain_size = 1 / dynamic_props.size;
    const domain_separator = 0.05 * domain_size;
    let start_domain = domain_size * dynamic_props.size - domain_size;
    for (const [name, prop] of dynamic_props) {
      const gen = chart.ChartManager.get_instance().get_chart_generator(prop.get_type().get_name());
      const c = gen.make_chart(prop, values.has(name) ? values.get(name)! : []);
      this.charts.set(name, c);
      const layout = { title: { text: name }, domain: [start_domain + domain_separator, start_domain + domain_size - domain_separator], zeroline: false, showticklabels: gen.show_tick_labels(), showgrid: gen.show_grid(), range: c.get_range() };
      if (i == 1) {
        this.yaxis.set(name, 'y');
        this.layout['yaxis'] = layout;
      }
      else {
        this.yaxis.set(name, `y${i}`);
        this.layout[`yaxis${i}`] = layout;
      }
      const yaxis = this.yaxis.get(name);
      for (const d of c.get_data()) {
        d.yaxis = yaxis;
        data.push(d);
      }
      i--;
      start_domain -= domain_size;
    }
    Plotly.newPlot(this.node, data, this.layout, this.config);
  }

  override unmounting(): void { this.payload.remove_item_listener(this); }

  types_updated(_: coco.taxonomy.Item): void { }
  properties_updated(_: coco.taxonomy.Item): void { }
  values_updated(_: coco.taxonomy.Item): void {
    const values: Map<string, chart.Value<unknown>[]> = new Map();
    for (const val of this.payload.get_data())
      for (const [name, v] of Object.entries(val.data)) {
        if (!values.has(name))
          values.set(name, []);
        values.get(name)!.push({ value: v, timestamp: val.timestamp });
      }

    const data: Partial<PlotData>[] = [];
    for (const [name, ch] of this.charts) {
      ch.set_data(values.has(name) ? values.get(name)! : []);
      const yaxis = this.yaxis.get(name);
      for (const d of ch.get_data()) {
        d.yaxis = yaxis;
        data.push(d);
      }
    }
    Plotly.react(this.node, data, this.layout, this.config);
  }
  new_value(_: coco.taxonomy.Item, val: coco.taxonomy.Datum): void {
    const data: Partial<PlotData>[] = [];
    for (const [name, ch] of this.charts) {
      if (name in val.data) // we update the chart with the new value
        ch.set_datum({ value: val.data[name], timestamp: val.timestamp });
      else // we extend the last data point with the new timestamp
        ch.extend(val.timestamp);
      const yaxis = this.yaxis.get(name);
      for (const d of ch.get_data()) {
        d.yaxis = yaxis;
        data.push(d);
      }
    }
    Plotly.react(this.node, data, this.layout, this.config);
  }
  slots_updated(_: coco.taxonomy.Item): void { }
}
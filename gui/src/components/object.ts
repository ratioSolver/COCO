import { h, VNode } from "snabbdom";
import { coco } from "../coco";
import { flick, Header, ListGroup, Row, Table } from "@ratiosolver/flick";
import { CoCoClass } from "./class";
import * as echarts from 'echarts/core';
import { LineChart } from 'echarts/charts';
import { LegendComponent, TooltipComponent, GridComponent, DataZoomComponent, AxisPointerComponent } from 'echarts/components';
import { CanvasRenderer } from 'echarts/renderers';
import { CustomSeriesRenderItemAPI, CustomSeriesRenderItemParams } from "echarts";

echarts.use([LineChart, LegendComponent, TooltipComponent, GridComponent, DataZoomComponent, AxisPointerComponent, CanvasRenderer]);

const PIXELS_PER_ROW = 150;
const MARGIN_TOP = 40;
const GAP = 50;

const obj_item_listener = {
  class_added: (_cls: coco.CoCoClass) => { },
  properties_updated: (properties: Record<string, coco.Value>) => { if (properties.name) flick.redraw(); },
  values_added: (_values: Record<string, coco.Value>, _date_time: string) => { },
  data_updated: (_data: Record<string, Array<coco.TimeValue>>) => { }
};

export function ObjectGroupItem(obj: coco.CoCoObject): VNode {
  const active = flick.ctx.page_title === `Object: ${obj.get_id()}`;
  return h('button.list-group-item.list-group-item-action' + (active ? '.active.rounded' : ''), {
    hook: {
      insert: () => {
        obj.add_listener(obj_item_listener);
      },
      destroy: () => {
        obj.remove_listener(obj_item_listener);
      }
    },
    props: { type: 'button' },
    attrs: { 'aria-current': active ? 'true' : 'false' },
    on: {
      click: () => {
        flick.ctx.current_page = () => CoCoObject(obj);
        flick.ctx.page_title = `Object: ${obj.get_id()}`;
        flick.redraw();
      }
    }
  }, object_to_string(obj));
}

export function ObjectsList(coco: coco.CoCo): VNode {
  return ListGroup(Array.from(coco.get_objects().values().map(obj => ObjectGroupItem(obj))));
}

const obj_listener = {
  class_added: (_cls: coco.CoCoClass) => { flick.redraw(); },
  properties_updated: (_properties: Record<string, coco.Value>) => { flick.redraw(); },
  values_added: (_values: Record<string, coco.Value>, _date_time: string) => { },
  data_updated: (_data: Record<string, Array<coco.TimeValue>>) => { }
};

let chart: ObjectChart | null = null;

export function CoCoObject(obj: coco.CoCoObject): VNode {
  const props_header = ["Property", "Value"];
  const props = obj.get_properties();
  const props_rows = props ? Object.entries(props).map(([name, value]) => Row([name, coco.value_to_string(value)])) : [];
  const data = obj.get_data();
  if (Object.keys(data).length === 0)
    obj.load_data();

  const vals = obj.get_values();
  const content = h('div.container.mt-2',
    {
      hook: {
        insert: () => {
          obj.add_listener(obj_listener);
        },
        destroy: () => {
          obj.remove_listener(obj_listener);
        }
      }
    }, [
    h('div.input-group', [
      h('input.form-control', { attrs: { type: 'text', value: obj.get_id(), placeholder: 'Type name', disabled: true } }),
      h('button.btn.btn-outline-secondary', {
        attrs: { type: 'button', title: 'Copy type name to clipboard' },
        on: { click: () => navigator.clipboard.writeText(obj.get_id()) }
      }, h('i.fa-solid.fa-copy')),
    ]),
    h('div.mt-2', Array.from(obj.get_classes()).map(cls =>
      h('span.badge.bg-primary.me-1', {
        style: { cursor: 'pointer' },
        on: {
          click: () => {
            flick.ctx.current_page = () => CoCoClass(cls);
            flick.ctx.page_title = `Class: ${cls.get_name()}`;
            flick.redraw();
          }
        }
      }, cls.get_name())
    )),
    props_rows.length > 0 ? Table(Header(props_header), props_rows, 'Properties') : h('p.mt-2', 'No properties.'),
    h('div.mt-2', {
      style: { minHeight: `${vals ? (Object.values(vals).length * PIXELS_PER_ROW) + MARGIN_TOP + GAP : 30}px` },
      hook: {
        insert: (vnode) => {
          chart = new ObjectChart(vnode.elm as HTMLDivElement, obj);
        },
        destroy: () => {
          if (chart) {
            chart.dispose();
            chart = null;
          }
        }
      }
    }, 'Loading history...')
  ]);
  return content;
}

function object_to_string(obj: coco.CoCoObject): string {
  return obj.get_properties()?.name as string || obj.get_id();
}

class ObjectChart implements coco.CoCoObjectListener {

  private readonly chart: echarts.ECharts;
  private readonly obj: coco.CoCoObject;

  constructor(node: HTMLDivElement, obj: coco.CoCoObject) {
    this.chart = echarts.init(node);
    this.obj = obj;
    this.obj.add_listener(this);
  }

  class_added(_cls: coco.CoCoClass): void { }
  properties_updated(_properties: Record<string, coco.Value>): void { }
  values_added(_values: Record<string, coco.Value>, _date_time: string): void { }
  data_updated(data: Record<string, Array<coco.TimeValue>>): void {
    const n_series = Object.keys(data).length;
    const series = Object.entries(data).map(([name, values], index) => create_property_chart(this.obj, index, n_series, name, values));

    const options = {
      tooltip: {
        trigger: 'axis',
      },
      dataZoom: [{ type: 'slider', xAxisIndex: Array.from(Array(n_series).keys()) }],
      grid: [] as any[],
      xAxis: [] as any[],
      yAxis: [] as any[],
      series: [] as any[],
    };
    for (const option of series) {
      options.grid.push(option.grid);
      options.xAxis.push(option.xAxis);
      options.yAxis.push(option.yAxis);
      options.series.push(option.series);
    }
    this.chart.setOption(options);
  }

  dispose() {
    this.obj.remove_listener(this);
    this.chart.dispose();
  }
}

function create_property_chart(obj: coco.CoCoObject, index: number, total: number, name: string, data: Array<coco.TimeValue>): echarts.EChartsCoreOption {
  const prop = get_property_type(obj, name);
  switch (prop.type) {
    case 'int':
    case 'float':
      return create_line_chart(index, total, name, prop, data);
    case 'bool':
    case 'string':
    case 'symbol':
    case 'object':
      return create_symbol_chart(index, total, name, prop, data);
    default:
      throw new Error(`Unsupported property type for chart: ${get_property_type(obj, name)}`);
  }
}

function create_line_chart(index: number, total: number, name: string, prop: coco.Property, data: Array<coco.TimeValue>): echarts.EChartsCoreOption {
  const top = MARGIN_TOP + (index * PIXELS_PER_ROW);
  const height = PIXELS_PER_ROW - GAP;
  return {
    grid: {
      top: top,
      height: height,
      left: 30,
      right: 10
    },
    xAxis: {
      type: 'time',
      gridIndex: index,
      show: index === total - 1, // Only bottom chart shows labels
      axisPointer: { show: true, snap: true, lineStyle: { color: '#999', width: 2 } }
    },
    yAxis: {
      type: 'value',
      gridIndex: index,
      name,
      min: prop.type === 'int' ? (prop.min ? prop.min as number - 1 : undefined) : undefined,
      max: prop.type === 'int' ? (prop.max ? prop.max as number + 1 : undefined) : undefined,
      splitLine: { show: true }
    },
    series: {
      name,
      type: 'line',
      xAxisIndex: index,
      yAxisIndex: index,
      data: data.map(d => [d.timestamp, d.value])
    }
  }
}

function create_symbol_chart(index: number, total: number, name: string, prop: coco.Property, data: Array<coco.TimeValue>): echarts.EChartsCoreOption {
  const top = MARGIN_TOP + (index * PIXELS_PER_ROW);
  const height = PIXELS_PER_ROW - GAP;
  return {
    grid: {
      top: top,
      height: height,
      left: 30,
      right: 10
    },
    xAxis: {
      type: 'time',
      gridIndex: index,
      show: index === total - 1, // Only bottom chart shows labels
      axisPointer: { show: true, snap: true, lineStyle: { color: '#999', width: 2 } }
    },
    yAxis: {
      type: 'value',
      gridIndex: index,
      name,
      splitLine: { show: true }
    },
    series: {
      name,
      type: 'custom',
      xAxisIndex: index,
      yAxisIndex: index,
      renderItem: (params: CustomSeriesRenderItemParams, api: CustomSeriesRenderItemAPI) => {
        const coordSys = params.coordSys as unknown as {
          x: number;
          y: number;
          width: number;
          height: number;
        };

        const start = api.coord([api.value(0), 0]);
        const end = api.coord([api.value(1), 0]);

        return {
          type: 'rect',
          shape: {
            x: start[0],
            y: coordSys.y,
            width: Math.max(0, end[0] - start[0]), // Ensure width isn't negative
            height: coordSys.height
          },
          style: {
            fill: api.visual('color')
          }
        };
      },
      encode: { x: [0, 1], y: 2 },
      data: data // Ensure this is [[start, end, "VALUE"], ...]
    }
  }
}

function get_property_type(obj: coco.CoCoObject, prop_name: string): coco.Property {
  for (const cls of obj.get_classes().values()) {
    const prop = cls.get_dynamic_properties().get(prop_name);
    if (prop) return prop;
  }
  throw new Error(`Property ${prop_name} not found in object ${obj.get_id()}`);
}
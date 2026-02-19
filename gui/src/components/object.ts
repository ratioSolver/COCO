import { h, VNode } from "snabbdom";
import { coco } from "../coco";
import { flick, Header, ListGroup, Row, Table } from "@ratiosolver/flick";
import { CoCoClass } from "./class";
import * as echarts from 'echarts/core';
import { LineChart } from 'echarts/charts';
import { LegendComponent, TooltipComponent, GridComponent, DataZoomComponent, AxisPointerComponent } from 'echarts/components';
import { CanvasRenderer } from 'echarts/renderers';

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

export function CoCoObject(obj: coco.CoCoObject): VNode {
  const props_header = ["Property", "Value"];
  const props = obj.get_properties();
  const props_rows = props ? Object.entries(props).map(([name, value]) => Row([name, coco.value_to_string(value)])) : [];
  const data = obj.get_data();
  const data_keys = Object.keys(data); // These are your actual data properties
  const num_layers = data_keys.length;
  if (num_layers === 0)
    obj.load_data();

  let chart: echarts.ECharts | undefined;

  const get_option = (): echarts.EChartsCoreOption => {
    const data = obj.get_data();
    const data_keys = Object.keys(data); // These are your actual data properties
    const num_layers = data_keys.length;

    if (num_layers === 0) return {}; // Handle empty state

    const rowHeight = 100 / num_layers;

    return {
      axisPointer: { link: [{ xAxisIndex: 'all' }] },
      tooltip: { trigger: 'axis' },
      grid: data_keys.map((_, i) => ({
        left: 20,
        right: 10,
        height: `${rowHeight - 15}%`,
        top: `${i * rowHeight + 10}%`
      })),
      xAxis: data_keys.map((_, i) => ({
        type: 'time',
        gridIndex: i,
        show: i === data_keys.length - 1,
      })),
      yAxis: data_keys.map((name, i) => ({
        type: 'value',
        gridIndex: i,
        name: name,
        splitLine: { show: true }
      })),
      series: data_keys.map((name, i) => ({
        type: 'line',
        xAxisIndex: i,
        yAxisIndex: i,
        data: data[name].map(d => [d.timestamp, d.value as number])
      })),
    };
  }

  const obj_listener = {
    class_added: (_cls: coco.CoCoClass) => { flick.redraw(); },
    properties_updated: (_properties: Record<string, coco.Value>) => { flick.redraw(); },
    values_added: (_values: Record<string, coco.Value>, _date_time: string) => { flick.redraw(); if (chart) chart.setOption(get_option()); },
    data_updated: (_data: Record<string, Array<coco.TimeValue>>) => { flick.redraw(); if (chart) chart.setOption(get_option()); }
  };

  let resize_handler: () => void;

  const vals = obj.get_values();
  const content = h('div.container.mt-2', [
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
          chart = echarts.init(vnode.elm as HTMLDivElement);
          chart.setOption(get_option());

          resize_handler = () => chart?.resize();
          window.addEventListener('resize', resize_handler);

          obj.add_listener(obj_listener);
        },
        destroy: () => {
          window.removeEventListener('resize', resize_handler);
          obj.remove_listener(obj_listener);
          if (chart) {
            chart.dispose();
            chart = undefined;
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

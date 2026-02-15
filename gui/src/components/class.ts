import { h, VNode } from "snabbdom";
import { coco } from "../coco";
import { flick, Header, ListGroup, ListGroupItem, Table } from "@ratiosolver/flick";
import { CoCoObject } from "./object";

export function ClassesList(coco: coco.CoCo): VNode {
  return ListGroup(Array.from(coco.get_classes().values().map(cls => ListGroupItem(cls.get_name(), () => {
    flick.ctx.current_page = CoCoClass(cls);
    flick.ctx.page_title = `Class: ${cls.get_name()}`;
    flick.redraw();
  }, flick.ctx.page_title === `Class: ${cls.get_name()}`))));
}

const cls_listener = {
  instance_added: (_obj: coco.CoCoObject) => {
    flick.redraw();
  }
};

const obj_item_listener = {
  class_added: (_cls: coco.CoCoClass) => { },
  properties_updated: (_properties: Record<string, coco.Value>) => { flick.redraw(); },
  values_added: (_values: Record<string, coco.Value>, _date_time: string) => { flick.redraw(); }
};

export function ObjectRow(cls: coco.CoCoClass, obj: coco.CoCoObject): VNode {
  const cells = [obj.get_id()];
  for (const prop of cls.get_static_properties().keys().toArray().sort()) {
    const props = obj.get_properties();
    if (props && prop in props)
      cells.push(coco.value_to_string(props[prop]));
    else
      cells.push('');
  }
  for (const prop of cls.get_dynamic_properties().keys().toArray().sort()) {
    const props = obj.get_values();
    if (props && prop in props)
      cells.push(coco.value_to_string(props[prop][0]));
    else
      cells.push('');
  }
  return h('tr', {
    hook: {
      insert: () => {
        obj.add_listener(obj_item_listener);
      },
      destroy: () => {
        obj.remove_listener(obj_item_listener);
      }
    },
    style: { cursor: 'pointer' },
    on: {
      click: () => {
        flick.ctx.current_page = CoCoObject(obj);
        flick.ctx.page_title = `Object: ${obj.get_id()}`;
        flick.redraw();
      }
    }
  }, cells.map(cell => h('td', cell)));
}

export function CoCoClass(cls: coco.CoCoClass): VNode {
  const header = ["ID", ...cls.get_static_properties().keys().toArray().sort(), ...cls.get_dynamic_properties().keys().toArray().sort()];
  const rows = cls.get_instances().values().map(obj => ObjectRow(cls, obj)).toArray();

  return h('div.container.mt-2',
    {
      hook: {
        insert: () => {
          cls.add_listener(cls_listener);
        },
        destroy: () => {
          cls.remove_listener(cls_listener);
        }
      }
    }, [
    h('div.input-group', [
      h('input.form-control', { attrs: { type: 'text', value: cls.get_name(), placeholder: 'Type name', disabled: true } }),
      h('button.btn.btn-outline-secondary', {
        attrs: { type: 'button', title: 'Copy type name to clipboard' },
        on: { click: () => navigator.clipboard.writeText(cls.get_name()) }
      }, h('i.fa-solid.fa-copy')),
    ]),
    rows.length > 0 ? Table(Header(header), rows, 'Instances') : h('p.mt-2', 'No instances of this class yet.')
  ]);
}
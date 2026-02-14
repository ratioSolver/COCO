import { h, VNode } from "snabbdom";
import { coco } from "../coco";
import { flick, Header, ListGroup, ListGroupItem, Row, Table } from "@ratiosolver/flick";
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

export function CoCoClass(cls: coco.CoCoClass): VNode {
  const header = ["ID", ...cls.get_static_properties().keys().toArray().sort(), ...cls.get_dynamic_properties().keys().toArray().sort()];
  const rows = cls.get_instances().values().map(obj => {
    const row = [obj.get_id()];
    for (const prop of cls.get_static_properties().keys().toArray().sort()) {
      const props = obj.get_properties();
      if (props && prop in props)
        row.push(coco.value_to_string(props[prop]));
      else
        row.push('');
    }
    for (const prop of cls.get_dynamic_properties().keys().toArray().sort()) {
      const props = obj.get_values();
      if (props && prop in props)
        row.push(coco.value_to_string(props[prop][0]));
      else
        row.push('');
    }
    return Row(row, () => {
      flick.ctx.current_page = CoCoObject(obj);
      flick.ctx.page_title = `Object: ${obj.get_id()}`;
      flick.redraw();
    });
  }).toArray();

  const content = h('div.container.mt-2',
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
  return content;
}
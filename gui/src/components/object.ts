import { h, VNode } from "snabbdom";
import { coco } from "../coco";
import { flick, ListGroup, ListGroupItem } from "@ratiosolver/flick";

export function ObjectsList(coco: coco.CoCo): VNode {
  return ListGroup(Array.from(coco.get_objects().values().map(obj => ListGroupItem(object_to_string(obj), () => {
    flick.ctx.current_page = Object(obj);
    flick.ctx.page_title = `Object: ${obj.get_id()}`;
    flick.redraw();
  }, flick.ctx.page_title === `Object: ${obj.get_id()}`))));
}

export function Object(obj: coco.CoCoObject): VNode {
  const content = h('div.container.mt-2.text-center', [
    h('div.input-group', [
      h('input.form-control', { attrs: { type: 'text', value: obj.get_id(), placeholder: 'Type name', disabled: true } }),
      h('button.btn.btn-outline-secondary', {
        attrs: { type: 'button', title: 'Copy type name to clipboard' },
        on: { click: () => navigator.clipboard.writeText(obj.get_id()) }
      }, h('i.fa-solid.fa-copy')),
    ]),
  ]);
  return content;
}

function object_to_string(obj: coco.CoCoObject): string {
  return obj.get_properties()?.name as string || obj.get_id();
}
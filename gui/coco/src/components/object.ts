import { h, VNode } from "snabbdom";
import { coco } from "../coco";
import { flick, ListGroup, ListGroupItem } from "@ratiosolver/flick";

export function ObjectsList(coco: coco.CoCo): VNode {
  return ListGroup(Array.from(coco.get_objects().values().map(obj => ListGroupItem(object_to_string(obj), () => {
    flick.ctx.current_page = Object(obj);
  }))));
}

export function Object(obj: coco.CoCoObject): VNode {
  const content = h('div.container.mt-5.text-center', [
    h('div', [
      h('input.form-control', { attrs: { type: 'text', value: obj.get_id(), placeholder: 'Object ID', readonly: 'readonly' } }),
      h('button.btn.btn-outline-secondary', {
        on: { click: () => navigator.clipboard.writeText(obj.get_id()) }
      }, [h('i.fa-regular.fa-copy.me-2'), 'Copy object ID']),
    ]),
  ]);
  return content;
}

function object_to_string(obj: coco.CoCoObject): string {
  return obj.get_properties()?.name as string || obj.get_id();
}
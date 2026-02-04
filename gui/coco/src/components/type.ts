import { h, VNode } from "snabbdom";
import { coco } from "../coco";
import { ListGroup, ListGroupItem } from "@ratiosolver/flick";

export function TypesList(coco: coco.CoCo): VNode {
  return ListGroup(Array.from(coco.get_types().values().map(type => ListGroupItem(type.get_name(), () => {
    console.log('Clicked on type', type.get_name());
  }))));
}

export function Type(type: coco.Type): VNode {
  const content = h('div.container.mt-5.text-center', [
    h('div', [
      h('input.form-control', { attrs: { type: 'text', value: type.get_name(), placeholder: 'Type Name', readonly: 'readonly' } }),
      h('button.btn.btn-outline-secondary', {
        on: { click: () => navigator.clipboard.writeText(type.get_name()) }
      }, [h('i.fa-regular.fa-copy.me-2'), 'Copy Type Name']),
    ]),
  ]);
  return content;
}
import { h, VNode } from "snabbdom";
import { coco } from "../coco";
import { flick, ListGroup, ListGroupItem } from "@ratiosolver/flick";

export function ClassesList(coco: coco.CoCo): VNode {
  return ListGroup(Array.from(coco.get_classes().values().map(cls => ListGroupItem(cls.get_name(), () => {
    flick.ctx.current_page = Class(cls);
  }))));
}

export function Class(cls: coco.CoCoClass): VNode {
  const content = h('div.container.mt-5.text-center', [
    h('div', [
      h('input.form-control', { attrs: { type: 'text', value: cls.get_name(), placeholder: 'Class Name', readonly: 'readonly' } }),
      h('button.btn.btn-outline-secondary', {
        on: { click: () => navigator.clipboard.writeText(cls.get_name()) }
      }, [h('i.fa-regular.fa-copy.me-2'), 'Copy class name']),
    ]),
  ]);
  return content;
}
import { h, VNode } from "snabbdom";
import { coco } from "../coco";
import { flick, ListGroup, ListGroupItem } from "@ratiosolver/flick";

export function ClassesList(coco: coco.CoCo): VNode {
  return ListGroup(Array.from(coco.get_classes().values().map(cls => ListGroupItem(cls.get_name(), () => {
    console.log('Clicked on class', cls.get_name());

    flick.ctx.current_page = Class(cls);
    flick.ctx.page_title = `Class: ${cls.get_name()}`;
    flick.redraw();
  }, flick.ctx.page_title === `Class: ${cls.get_name()}`))));
}

export function Class(cls: coco.CoCoClass): VNode {
  const content = h('div.container.mt-2', [
    h('div.input-group', [
      h('input.form-control', { attrs: { type: 'text', value: cls.get_name(), placeholder: 'Type name', disabled: true } }),
      h('button.btn.btn-outline-secondary', {
        attrs: { type: 'button', title: 'Copy type name to clipboard' },
        on: { click: () => navigator.clipboard.writeText(cls.get_name()) }
      }, h('i.fa-solid.fa-copy')),
    ]),
  ]);
  return content;
}
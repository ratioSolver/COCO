import { h, VNode } from "snabbdom";
import { coco } from "../coco";
import { flick, ListGroup, ListGroupItem } from "@ratiosolver/flick";

export function RulesList(coco: coco.CoCo): VNode {
  return ListGroup(Array.from(coco.get_rules().values().map(rule => ListGroupItem(rule.get_name(), () => {
    flick.ctx.current_page = Rule(rule);
    flick.ctx.page_title = `Rule: ${rule.get_name()}`;
    flick.redraw();
  }, flick.ctx.page_title === `Rule: ${rule.get_name()}`))));
}

export function Rule(rule: coco.CoCoRule): VNode {
  const content = h('div.container.mt-2.h-100', [
    h('div.input-group', [
      h('input.form-control', { attrs: { type: 'text', value: rule.get_name(), placeholder: 'Rule name', disabled: true } }),
      h('button.btn.btn-outline-secondary', {
        attrs: { type: 'button', title: 'Copy rule name to clipboard' },
        on: { click: () => navigator.clipboard.writeText(rule.get_name()) }
      }, h('i.fa-solid.fa-copy')),
    ]),
  ]);
  return content;
}
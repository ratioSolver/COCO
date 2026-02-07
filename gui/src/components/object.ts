import { h, VNode } from "snabbdom";
import { coco } from "../coco";
import { flick, ListGroup, ListGroupItem } from "@ratiosolver/flick";
import { Class } from "./class";

export function ObjectsList(coco: coco.CoCo): VNode {
  return ListGroup(Array.from(coco.get_objects().values().map(obj => ListGroupItem(object_to_string(obj), () => {
    flick.ctx.current_page = Object(obj);
    flick.ctx.page_title = `Object: ${obj.get_id()}`;
    flick.redraw();
  }, flick.ctx.page_title === `Object: ${obj.get_id()}`))));
}

const obj_listener = {
  class_added: (_cls: coco.CoCoClass) => {
    flick.redraw();
  },
  properties_updated: (_properties: Record<string, coco.Value>) => {
    flick.redraw();
  },
  values_added: (_values: Record<string, coco.Value>, _date_time: string) => {
    flick.redraw();
  }
};

export function Object(obj: coco.CoCoObject): VNode {
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
            flick.ctx.current_page = Class(cls);
            flick.ctx.page_title = `Class: ${cls.get_name()}`;
            flick.redraw();
          }
        }
      }, cls.get_name())
    )),
  ]);
  return content;
}

function object_to_string(obj: coco.CoCoObject): string {
  return obj.get_properties()?.name as string || obj.get_id();
}
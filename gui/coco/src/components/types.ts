import { VNode } from "snabbdom";
import { CoCo } from "../coco";
import { ListGroup, ListGroupItem } from "@ratiosolver/flick";

export function Types(coco: CoCo): VNode {
  return ListGroup(Array.from(coco.get_types().values().map(type => ListGroupItem(type.get_name(), () => {
    console.log('Clicked on type', type.get_name());
  }))));
}
import { VNode } from "snabbdom";
import { CoCo, Object } from "../coco";
import { ListGroup, ListGroupItem } from "@ratiosolver/flick";

export function Objects(coco: CoCo): VNode {
  return ListGroup(Array.from(coco.get_objects().values().map(obj => ListGroupItem(object_to_string(obj), () => {
    console.log('Clicked on object', object_to_string(obj));
  }))));
}

function object_to_string(obj: Object): string {
  return obj.get_properties()?.name as string || obj.get_id();
}
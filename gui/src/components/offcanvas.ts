import { h, VNode } from "snabbdom";
import { coco } from "../coco";
import { OffcanvasBody, Offcanvas as OffcanvasComponent } from "@ratiosolver/flick";
import { ClassesList } from "./class";
import { ObjectsList } from "./object";

export function Offcanvas(coco: coco.CoCo): VNode {
    return OffcanvasComponent(
        OffcanvasBody([
            coco.get_classes().size > 0 ? h('label', 'Classes') : null,
            ClassesList(coco),
            coco.get_objects().size > 0 ? h('label', 'Objects') : null,
            ObjectsList(coco)
        ])
    );
}
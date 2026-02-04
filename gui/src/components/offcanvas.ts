import { VNode } from "snabbdom";
import { coco } from "../coco";
import { OffcanvasBody, Offcanvas as OffcanvasComponent } from "@ratiosolver/flick";
import { ClassesList } from "./class";
import { ObjectsList } from "./object";

export function Offcanvas(coco: coco.CoCo): VNode {
    return OffcanvasComponent(
        OffcanvasBody([
            ClassesList(coco),
            ObjectsList(coco)
        ]
        )
    );
}
import { Component, UListComponent } from "ratio-core";
import { coco } from "../coco";

export class TypeElement extends Component<coco.taxonomy.Type, HTMLLIElement> {

	a: HTMLAnchorElement;

	constructor(type: coco.taxonomy.Type) {
		super(type, document.createElement('li'));

		this.a = document.createElement('a');

		this.element.append(this.a);
	}
}

export class TypeList extends UListComponent<coco.taxonomy.Type> implements coco.CoCoListener {

	constructor(tps: coco.taxonomy.Type[] = []) {
		super(tps.map(tp => new TypeElement(tp)), (t0: coco.taxonomy.Type, t1: coco.taxonomy.Type) => t0.get_name() === t1.get_name() ? 0 : (t0.get_name() < t1.get_name() ? -1 : 1));
	}

	new_type(type: coco.taxonomy.Type): void {
		const te = new TypeElement(type);
		this.add_child(te);
	}

	new_item(_: coco.taxonomy.Item): void { }
}
import { Component, UListComponent } from "ratio-core";
import { coco } from "../coco";

export class ItemElement extends Component<coco.taxonomy.Item, HTMLLIElement> {

	a: HTMLAnchorElement;

	constructor(type: coco.taxonomy.Item) {
		super(type, document.createElement('li'));

		this.a = document.createElement('a');

		this.element.append(this.a);
	}
}

export class ItemList extends UListComponent<coco.taxonomy.Item> implements coco.CoCoListener {

	constructor() {
		super([]);
	}

	new_type(_: coco.taxonomy.Type): void { }

	new_item(item: coco.taxonomy.Item): void {
		const te = new ItemElement(item);
		this.add_child(te);
	}
}
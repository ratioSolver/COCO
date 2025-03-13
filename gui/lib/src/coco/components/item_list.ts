import { Component, UListComponent } from "ratio-core";
import { coco } from "../coco";

export class ItemElement extends Component<coco.taxonomy.Item, HTMLLIElement> {

	a: HTMLAnchorElement;

	constructor(item: coco.taxonomy.Item) {
		super(item, document.createElement('li'));

		this.a = document.createElement('a');
		this.a.classList.add('nav-link');
		this.a.href = '#';
		const props = item.get_properties();
		if (props && 'name' in props)
			this.a.textContent = ' ' + props.name;
		else
			this.a.textContent = ' ' + item.get_id();

		this.element.append(this.a);
	}
}

export class ItemList extends UListComponent<coco.taxonomy.Item> implements coco.CoCoListener {

	constructor(itms: coco.taxonomy.Item[] = []) {
		super(itms.map(itm => new ItemElement(itm)));
		this.element.classList.add('nav', 'nav-pills', 'flex-column');
		coco.CoCo.get_instance().add_coco_listener(this);
	}

	new_type(_: coco.taxonomy.Type): void { }
	type_updated(_: coco.taxonomy.Type): void { }

	new_item(item: coco.taxonomy.Item): void {
		const te = new ItemElement(item);
		this.add_child(te);
	}
}
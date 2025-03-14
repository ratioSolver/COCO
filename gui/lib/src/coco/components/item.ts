import { Component, UListComponent, Selector, SelectorGroup, App } from "ratio-core";
import { coco } from "../coco";

export class ItemElement extends Component<coco.taxonomy.Item, HTMLLIElement> implements coco.taxonomy.ItemListener, Selector {

  private group: SelectorGroup;
  private a: HTMLAnchorElement;

  constructor(group: SelectorGroup, item: coco.taxonomy.Item) {
    super(item, document.createElement('li'));
    this.group = group;
    this.element.classList.add('nav-item', 'list-group-item');

    this.a = document.createElement('a');
    this.a.classList.add('nav-link');
    this.a.href = '#';
    const props = item.get_properties();
    if (props && 'name' in props)
      this.a.textContent = ' ' + props.name;
    else
      this.a.textContent = ' ' + item.get_id();
    this.a.addEventListener('click', (event) => {
      event.preventDefault();
      App.get_instance().selected_component(new Item(this.payload));
      group.set_selected(this);
    });

    this.element.append(this.a);
    group.add_selector(this);
  }

  override unmounting(): void { this.group.remove_selector(this); }

  properties_updated(_: coco.taxonomy.Item): void { }
  values_updated(_: coco.taxonomy.Item): void { }
  new_value(_i: coco.taxonomy.Item, _v: coco.taxonomy.Value): void { }

  select(): void { this.a.classList.add('active'); }
  unselect(): void { this.a.classList.remove('active'); }
}

export class ItemList extends UListComponent<coco.taxonomy.Item> implements coco.CoCoListener {

  private group: SelectorGroup;

  constructor(group: SelectorGroup = new SelectorGroup(), itms: coco.taxonomy.Item[] = []) {
    super(itms.map(itm => new ItemElement(group, itm)));
    this.group = group;
    this.element.classList.add('nav', 'nav-pills', 'list-group', 'flex-column');
    coco.CoCo.get_instance().add_coco_listener(this);
  }

  override unmounting(): void { coco.CoCo.get_instance().remove_coco_listener(this); }

  new_type(_: coco.taxonomy.Type): void { }
  new_item(item: coco.taxonomy.Item): void { this.add_child(new ItemElement(this.group, item)); }
}

export class Item extends Component<coco.taxonomy.Item, HTMLDivElement> implements coco.taxonomy.ItemListener {

  constructor(item: coco.taxonomy.Item) {
    super(item, document.createElement('div'));
    item.add_item_listener(this);
  }

  override unmounting(): void { this.payload.remove_item_listener(this); }

  properties_updated(_: coco.taxonomy.Item): void { }
  values_updated(_: coco.taxonomy.Item): void { }
  new_value(_i: coco.taxonomy.Item, _v: coco.taxonomy.Value): void { }
}
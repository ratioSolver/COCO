import { Component, UListComponent, Selector, SelectorGroup, App } from "@ratiosolver/flick";
import { coco } from "../coco";
import { library, icon } from '@fortawesome/fontawesome-svg-core'
import { faCopy, faTag } from '@fortawesome/free-solid-svg-icons'
import { ItemProperties } from "./item_properties";
import { ItemChart } from "./item_chart";
import { ItemPublisher } from "./item_publisher";

library.add(faCopy, faTag);

export class ItemElement extends Component<coco.taxonomy.Item, HTMLLIElement> implements Selector {

  private group: SelectorGroup;
  private a: HTMLAnchorElement;

  constructor(group: SelectorGroup, item: coco.taxonomy.Item) {
    super(item, document.createElement('li'));
    this.group = group;
    this.element.classList.add('nav-item', 'list-group-item');

    this.a = document.createElement('a');
    this.a.classList.add('nav-link', 'd-flex', 'align-items-center');
    this.a.href = '#';
    const icn = icon(faTag).node[0];
    icn.classList.add('me-2');
    this.a.append(icn);
    this.a.append(document.createTextNode(item.to_string()));
    this.a.addEventListener('click', (event) => {
      event.preventDefault();
      group.set_selected(this);
    });

    this.element.append(this.a);
    group.add_selector(this);
  }

  override unmounting(): void { this.group.remove_selector(this); }

  select(): void {
    this.a.classList.add('active');
    App.get_instance().selected_component(new Item(this.payload));
  }
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
  new_intent(_: coco.llm.Intent): void { }
  new_entity(_: coco.llm.Entity): void { }
}

export class Item extends Component<coco.taxonomy.Item, HTMLDivElement> {

  constructor(item: coco.taxonomy.Item) {
    super(item, document.createElement('div'));
    this.element.classList.add('d-flex', 'flex-column', 'flex-grow-1');
    this.element.style.margin = '1em';

    const id_div = document.createElement('div');
    id_div.classList.add('input-group');
    const id_input = document.createElement('input');
    id_input.classList.add('form-control');
    id_input.type = 'text';
    id_input.placeholder = 'Item ID';
    id_input.value = item.get_id();
    id_input.disabled = true;
    id_div.append(id_input);
    const id_button_div = document.createElement('div');
    id_button_div.classList.add('input-group-append');
    const id_button = document.createElement('button');
    id_button.classList.add('btn', 'btn-outline-secondary');
    id_button.type = 'button';
    id_button.append(icon(faCopy).node[0]);
    id_button.title = 'Copy item ID to clipboard';
    id_button.addEventListener('click', () => {
      navigator.clipboard.writeText(item.get_id());
    });
    id_button_div.append(id_button);
    id_div.append(id_button_div);
    this.element.append(id_div);

    this.add_child(new ItemProperties(item));

    coco.CoCo.get_instance().load_data(item);
  }

  override mounted(): void {
    if (this.payload.get_type().get_all_dynamic_properties().size > 0) {
      this.add_child(new ItemChart(this.payload));
      this.add_child(new ItemPublisher(this.payload));
    }
  }
}
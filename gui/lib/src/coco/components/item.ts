import { Component, UListComponent, Selector, SelectorGroup, App } from "ratio-core";
import { coco } from "../coco";
import { library, icon } from '@fortawesome/fontawesome-svg-core'
import { faCopy, faTag } from '@fortawesome/free-solid-svg-icons'

library.add(faCopy, faTag);

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
    this.a.textContent = icon(faTag).html[0] + ' ' + item.to_string();
    this.a.addEventListener('click', (event) => {
      event.preventDefault();
      group.set_selected(this);
    });

    this.element.append(this.a);
    group.add_selector(this);
  }

  override unmounting(): void { this.group.remove_selector(this); }

  properties_updated(_: coco.taxonomy.Item): void { }
  values_updated(_: coco.taxonomy.Item): void { }
  new_value(_i: coco.taxonomy.Item, _v: coco.taxonomy.Value): void { }

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
}

export class Item extends Component<coco.taxonomy.Item, HTMLDivElement> implements coco.taxonomy.ItemListener {

  private p_label: HTMLLabelElement;
  private p_body: HTMLTableSectionElement;

  constructor(item: coco.taxonomy.Item) {
    super(item, document.createElement('div'));
    this.element.classList.add('d-flex', 'flex-column', 'flex-grow-1');

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
    id_button.innerHTML = icon(faCopy).html[0];
    id_button_div.append(id_button);
    id_div.append(id_button_div);
    this.element.append(id_div);

    this.p_label = document.createElement('label');
    this.p_label.title = 'Properties';

    const p_table = document.createElement('table');
    p_table.classList.add('table');

    const sthead = p_table.createTHead();
    const shrow = sthead.insertRow();
    const sproperty_name = shrow.insertCell();
    sproperty_name.scope = 'col';
    sproperty_name.textContent = 'Name';
    const sproperty_type = shrow.insertCell();
    sproperty_type.scope = 'col';
    sproperty_type.textContent = 'Value';
    this.p_body = p_table.createTBody();

    this.set_properties();

    this.element.append(p_table);

    item.add_item_listener(this);
  }

  override unmounting(): void { this.payload.remove_item_listener(this); }

  properties_updated(_: coco.taxonomy.Item): void { this.set_properties(); }
  values_updated(_: coco.taxonomy.Item): void { }
  new_value(_i: coco.taxonomy.Item, _v: coco.taxonomy.Value): void { }

  private set_properties() {
    this.p_body.innerHTML = '';
    const ps = this.payload.get_properties();
    if (ps && Object.keys(ps).length > 0) {
      this.p_label.hidden = false;
      const props = this.payload.get_type().get_all_static_properties();
      const fragment = document.createDocumentFragment();
      for (const [name, v] of Object.entries(ps)) {
        const row = document.createElement('tr');
        const dp_name = document.createElement('th');
        dp_name.scope = 'col';
        dp_name.textContent = name;
        row.appendChild(dp_name);
        const dp_type = document.createElement('td');
        dp_type.textContent = props.get(name)!.get_type().to_string(v);
        row.appendChild(dp_type);
        fragment.appendChild(row);
      }
      this.p_body.appendChild(fragment);
    } else
      this.p_label.hidden = true;
  }
}
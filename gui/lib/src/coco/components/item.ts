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
  private p_table: HTMLTableElement;
  private p_body: HTMLTableSectionElement;
  private v_label: HTMLLabelElement;
  private v_table: HTMLTableElement;
  private v_body: HTMLTableSectionElement;

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
    id_button.append(icon(faCopy).node[0]);
    id_button_div.append(id_button);
    id_div.append(id_button_div);
    this.element.append(id_div);

    this.p_label = document.createElement('label');
    this.p_label.title = 'Properties';
    this.element.append(this.p_label);

    this.p_table = document.createElement('table');
    this.p_table.classList.add('table');

    const p_thead = this.p_table.createTHead();
    const p_hrow = p_thead.insertRow();
    const p_name = document.createElement('th');
    p_name.scope = 'col';
    p_name.textContent = 'Name';
    p_hrow.appendChild(p_name);
    const p_val = document.createElement('th');
    p_val.scope = 'col';
    p_val.textContent = 'Value';
    p_hrow.appendChild(p_val);

    this.p_body = this.p_table.createTBody();

    this.v_label = document.createElement('label');
    this.v_label.title = 'Properties';
    this.element.append(this.v_label);

    this.v_table = document.createElement('table');
    this.v_table.classList.add('table');

    const v_thead = this.v_table.createTHead();
    const v_hrow = v_thead.insertRow();
    const v_name = document.createElement('th');
    v_name.scope = 'col';
    v_name.textContent = 'Name';
    v_hrow.appendChild(v_name);
    const v_val = document.createElement('th');
    v_val.scope = 'col';
    v_val.textContent = 'Value';
    v_hrow.appendChild(v_val);

    this.v_body = this.v_table.createTBody();

    this.set_properties();
    this.set_value();

    this.element.append(this.p_table);
    this.element.append(this.v_table);

    item.add_item_listener(this);
  }

  override unmounting(): void { this.payload.remove_item_listener(this); }

  properties_updated(_: coco.taxonomy.Item): void { this.set_properties(); }
  values_updated(_: coco.taxonomy.Item): void { }
  new_value(_i: coco.taxonomy.Item, _v: coco.taxonomy.Value): void { this.set_value(); }

  private set_properties() {
    this.p_body.innerHTML = '';
    const ps = this.payload.get_properties();
    if (ps && Object.keys(ps).length > 0) {
      this.p_label.hidden = false;
      this.p_table.hidden = false;
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
    } else {
      this.p_label.hidden = true;
      this.p_table.hidden = true;
    }
  }

  private set_value() {
    this.v_body.innerHTML = '';
    const val = this.payload.get_value();
    if (val && Object.keys(val).length > 0) {
      this.v_label.hidden = false;
      this.v_table.hidden = false;
      const props = this.payload.get_type().get_all_dynamic_properties();
      const fragment = document.createDocumentFragment();
      for (const [name, v] of Object.entries(val)) {
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
      this.v_body.appendChild(fragment);
    } else {
      this.v_label.hidden = true;
      this.v_table.hidden = true;
    }
  }
}
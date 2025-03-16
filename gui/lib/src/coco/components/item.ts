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

  private p_values = new Map<string, HTMLTableCellElement>();

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

    const p_label = document.createElement('label');
    p_label.title = 'Properties';
    this.element.append(p_label);

    const p_table = document.createElement('table');
    p_table.classList.add('table');

    const p_thead = p_table.createTHead();
    const p_hrow = p_thead.insertRow();
    const p_name = document.createElement('th');
    p_name.scope = 'col';
    p_name.textContent = 'Name';
    p_hrow.appendChild(p_name);
    const p_val = document.createElement('th');
    p_val.scope = 'col';
    p_val.textContent = 'Value';
    p_hrow.appendChild(p_val);

    const p_body = p_table.createTBody();
    const props = item.get_properties();
    for (const [name, prop] of item.get_type().get_all_static_properties()) {
      const row = p_body.insertRow();
      const p_name = document.createElement('th');
      p_name.scope = 'col';
      p_name.textContent = name;
      row.appendChild(p_name);
      const p_value = document.createElement('td');
      this.p_values.set(name, p_value);
      if (props && props[name])
        p_value.textContent = prop.get_type().to_string(props[name]);
      row.appendChild(p_value);
    }

    const v_label = document.createElement('label');
    v_label.title = 'Properties';
    this.element.append(v_label);

    const v_table = document.createElement('table');
    v_table.classList.add('table');

    const v_thead = v_table.createTHead();
    const v_hrow = v_thead.insertRow();
    const v_name = document.createElement('th');
    v_name.scope = 'col';
    v_name.textContent = 'Name';
    v_hrow.appendChild(v_name);
    const v_val = document.createElement('th');
    v_val.scope = 'col';
    v_val.textContent = 'Value';
    v_hrow.appendChild(v_val);

    const v_body = v_table.createTBody();
    const val = item.get_value();
    for (const [name, prop] of item.get_type().get_all_dynamic_properties()) {
      const row = v_body.insertRow();
      const v_name = document.createElement('th');
      v_name.scope = 'col';
      v_name.textContent = name;
      row.appendChild(v_name);
      const v_value = document.createElement('td');
      this.p_values.set(name, v_value);
      if (val && val.data[name])
        v_value.textContent = prop.get_type().to_string(val.data[name]);
      row.appendChild(v_value);
    }

    this.element.append(p_table);
    this.element.append(v_table);

    item.add_item_listener(this);
  }

  override unmounting(): void { this.payload.remove_item_listener(this); }

  properties_updated(_: coco.taxonomy.Item): void { this.set_properties(); }
  values_updated(_: coco.taxonomy.Item): void { }
  new_value(_i: coco.taxonomy.Item, _v: coco.taxonomy.Value): void { this.set_value(); }

  private set_properties() {
    const ps = this.payload.get_properties();
    if (ps)
      for (const [name, prop] of this.payload.get_type().get_all_static_properties())
        if (ps && ps[name])
          this.p_values.get(name)!.textContent = prop.get_type().to_string(ps[name]);
  }

  private set_value() {
    const val = this.payload.get_value();
    if (val)
      for (const [name, prop] of this.payload.get_type().get_all_dynamic_properties())
        if (val && val.data[name])
          this.p_values.get(name)!.textContent = prop.get_type().to_string(val.data[name]);
  }
}
import { Component, UListComponent, Selector, SelectorGroup, App, blink } from "ratio-core";
import { coco } from "../coco";
import { library, icon } from '@fortawesome/fontawesome-svg-core'
import { faCopy, faTag } from '@fortawesome/free-solid-svg-icons'
import { publisher } from "./publisher";

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

  private readonly p_values = new Map<string, HTMLTableCellElement>();
  private readonly val: Record<string, unknown> = {};
  private readonly v_values = new Map<string, publisher.Publisher<unknown>>();

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
    id_button_div.append(id_button);
    id_div.append(id_button_div);
    this.element.append(id_div);

    if (item.get_type().get_all_static_properties().size > 0) {
      const p_table = document.createElement('table');
      p_table.createCaption().textContent = 'Properties';
      p_table.classList.add('table', 'caption-top');

      const p_thead = p_table.createTHead();
      const p_hrow = p_thead.insertRow();
      const p_name = document.createElement('th');
      p_name.scope = 'col';
      p_name.textContent = 'Name';
      p_name.classList.add('w-75');
      p_hrow.appendChild(p_name);
      const p_val = document.createElement('th');
      p_val.scope = 'col';
      p_val.textContent = 'Value';
      p_val.classList.add('w-25');
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

      this.element.append(p_table);
    }

    if (item.get_type().get_all_dynamic_properties().size > 0) {
      const v_table = document.createElement('table');
      v_table.createCaption().textContent = 'Values';
      v_table.classList.add('table', 'caption-top');

      const v_thead = v_table.createTHead();
      const v_hrow = v_thead.insertRow();

      const v_name = document.createElement('th');
      v_name.scope = 'col';
      v_name.textContent = 'Name';
      v_name.classList.add('w-75');
      v_hrow.appendChild(v_name);

      const v_sel_all = document.createElement('th');
      v_sel_all.scope = 'col';
      v_sel_all.classList.add('w-auto', 'text-center'); // Auto width and centered content
      const v_sel_all_check = document.createElement('input');
      v_sel_all_check.classList.add('form-check-input');
      v_sel_all_check.type = 'checkbox';
      v_sel_all_check.addEventListener('change', function () { });
      v_sel_all.appendChild(v_sel_all_check);
      v_hrow.appendChild(v_sel_all);

      const v_val = document.createElement('th');
      v_val.scope = 'col';
      v_val.textContent = 'Value';
      v_val.classList.add('w-25');
      v_hrow.appendChild(v_val);

      const v_body = v_table.createTBody();
      for (const [name, prop] of item.get_type().get_all_dynamic_properties()) {
        const row = v_body.insertRow();

        const v_name = document.createElement('th');
        v_name.scope = 'col';
        v_name.textContent = name;
        row.appendChild(v_name);

        const v_sel = document.createElement('td');
        v_sel.classList.add('text-center');
        const v_sel_check = document.createElement('input');
        v_sel_check.classList.add('form-check-input');
        v_sel_check.type = 'checkbox';
        v_sel_check.classList.add('property-checkbox');
        v_sel.appendChild(v_sel_check);
        row.appendChild(v_sel);

        const v_value = document.createElement('td');
        const pub = publisher.PublisherManager.get_instance().get_publisher_maker(prop.get_type().get_name()).make_publisher(name, prop, this.val);
        this.v_values.set(name, pub);
        v_value.append(pub.get_element());
        row.appendChild(v_value);
      }
      this.set_value();

      this.element.append(v_table);
    }

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
        if (ps && ps[name]) {
          const p_val = this.p_values.get(name)!;
          blink(p_val, 500);
          p_val.textContent = prop.get_type().to_string(ps[name]);
        }
  }

  private set_value() {
    const val = this.payload.get_value();
    if (val)
      for (const [name, _] of this.payload.get_type().get_all_dynamic_properties())
        if (val && val.data[name]) {
          const v_val = this.v_values.get(name)!;
          blink(v_val.get_element().children[0] as HTMLElement, 500);
          v_val.set_value(val.data[name]);
        }
  }
}
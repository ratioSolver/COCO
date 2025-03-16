import { Component, UListComponent, Selector, SelectorGroup, App } from "ratio-core";
import { coco } from "../coco";
import { library, icon } from '@fortawesome/fontawesome-svg-core'
import { faCopy, faCube } from '@fortawesome/free-solid-svg-icons'

library.add(faCopy, faCube);

export class TypeElement extends Component<coco.taxonomy.Type, HTMLLIElement> implements coco.taxonomy.TypeListener, Selector {

  private group: SelectorGroup;
  private a: HTMLAnchorElement;

  constructor(group: SelectorGroup, type: coco.taxonomy.Type) {
    super(type, document.createElement('li'));
    this.group = group;
    this.element.classList.add('nav-item', 'list-group-item');

    this.a = document.createElement('a');
    this.a.classList.add('nav-link', 'd-flex', 'align-items-center');
    this.a.href = '#';
    const icn = icon(faCube).node[0];
    icn.classList.add('me-2');
    this.a.append(icn);
    this.a.append(document.createTextNode(type.get_name()));
    this.a.addEventListener('click', (event) => {
      event.preventDefault();
      group.set_selected(this);
    });

    this.element.append(this.a);
    group.add_selector(this);
  }

  override unmounting(): void { this.group.remove_selector(this); }

  parents_updated(_: coco.taxonomy.Type): void { }
  data_updated(_: coco.taxonomy.Type): void { }
  static_properties_updated(_: coco.taxonomy.Type): void { }
  dynamic_properties_updated(_: coco.taxonomy.Type): void { }

  select(): void {
    this.a.classList.add('active');
    App.get_instance().selected_component(new Type(this.payload));
  }
  unselect(): void { this.a.classList.remove('active'); }
}

export class TypeList extends UListComponent<coco.taxonomy.Type> implements coco.CoCoListener {

  private group: SelectorGroup;

  constructor(group: SelectorGroup = new SelectorGroup(), tps: coco.taxonomy.Type[] = []) {
    super(tps.map(tp => new TypeElement(group, tp)), (t0: coco.taxonomy.Type, t1: coco.taxonomy.Type) => t0.get_name() === t1.get_name() ? 0 : (t0.get_name() < t1.get_name() ? -1 : 1));
    this.group = group;
    this.element.classList.add('nav', 'nav-pills', 'list-group', 'flex-column');
    coco.CoCo.get_instance().add_coco_listener(this);
  }

  override unmounting(): void { coco.CoCo.get_instance().remove_coco_listener(this); }

  new_type(type: coco.taxonomy.Type): void { this.add_child(new TypeElement(this.group, type)); }
  new_item(_: coco.taxonomy.Item): void { }
}

export class Type extends Component<coco.taxonomy.Type, HTMLDivElement> implements coco.taxonomy.TypeListener {

  private sp_label: HTMLLabelElement;
  private sp_table: HTMLTableElement;
  private sp_body: HTMLTableSectionElement;
  private dp_label: HTMLLabelElement;
  private dp_table: HTMLTableElement;
  private dp_body: HTMLTableSectionElement;

  constructor(type: coco.taxonomy.Type) {
    super(type, document.createElement('div'));
    this.element.classList.add('d-flex', 'flex-column', 'flex-grow-1');

    const id_div = document.createElement('div');
    id_div.classList.add('input-group');
    const id_input = document.createElement('input');
    id_input.classList.add('form-control');
    id_input.type = 'text';
    id_input.placeholder = 'Type name';
    id_input.value = type.get_name();
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

    this.sp_label = document.createElement('label');
    this.sp_label.title = 'Static properties';

    this.sp_table = document.createElement('table');
    this.sp_table.classList.add('table');

    const sthead = this.sp_table.createTHead();
    const shrow = sthead.insertRow();
    const sp_name = shrow.insertCell();
    sp_name.scope = 'col';
    sp_name.textContent = 'Name';
    const sp_type = shrow.insertCell();
    sp_type.scope = 'col';
    sp_type.textContent = 'Type';
    this.sp_body = this.sp_table.createTBody();

    this.dp_label = document.createElement('label');
    this.dp_label.title = 'Dynamic properties';

    this.dp_table = document.createElement('table');
    this.dp_table.classList.add('table');

    const dthead = this.dp_table.createTHead();
    const dhrow = dthead.insertRow();
    const dp_name = dhrow.insertCell();
    dp_name.scope = 'col';
    dp_name.textContent = 'Name';
    const dp_type = dhrow.insertCell();
    dp_type.scope = 'col';
    dp_type.textContent = 'Type';
    this.dp_body = this.dp_table.createTBody();

    this.set_static_properties();
    this.set_dynamic_properties();

    this.element.append(this.sp_table);
    this.element.append(this.dp_table);

    type.add_type_listener(this);
  }

  override unmounting(): void { this.payload.remove_type_listener(this); }

  parents_updated(_: coco.taxonomy.Type): void { }
  data_updated(_: coco.taxonomy.Type): void { }
  static_properties_updated(_: coco.taxonomy.Type): void { this.set_static_properties(); }
  dynamic_properties_updated(_: coco.taxonomy.Type): void { this.set_dynamic_properties(); }

  private set_static_properties() {
    this.sp_body.innerHTML = '';
    const sps = this.payload.get_static_properties();
    if (sps && sps.size > 0) {
      this.sp_label.hidden = false;
      this.sp_table.hidden = false;
      const fragment = document.createDocumentFragment();
      for (const [name, tp] of sps) {
        const row = document.createElement('tr');
        const sp_name = document.createElement('th');
        sp_name.scope = 'col';
        sp_name.textContent = name;
        row.appendChild(sp_name);
        const sp_type = document.createElement('td');
        sp_type.textContent = tp.to_string();
        row.appendChild(sp_type);
        fragment.appendChild(row);
      }
      this.sp_body.appendChild(fragment);
    } else {
      this.sp_label.hidden = true;
      this.sp_table.hidden = true;
    }
  }

  private set_dynamic_properties() {
    this.dp_body.innerHTML = '';
    const dps = this.payload.get_dynamic_properties();
    if (dps && dps.size > 0) {
      this.dp_label.hidden = false;
      this.dp_table.hidden = false;
      const fragment = document.createDocumentFragment();
      for (const [name, tp] of dps) {
        const row = document.createElement('tr');
        const dp_name = document.createElement('th');
        dp_name.scope = 'col';
        dp_name.textContent = name;
        row.appendChild(dp_name);
        const dp_type = document.createElement('td');
        dp_type.textContent = tp.to_string();
        row.appendChild(dp_type);
        fragment.appendChild(row);
      }
      this.dp_body.appendChild(fragment);
    } else {
      this.dp_label.hidden = true;
      this.dp_table.hidden = true;
    }
  }
}


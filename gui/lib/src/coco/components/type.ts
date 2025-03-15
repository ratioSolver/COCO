import { Component, UListComponent, Selector, SelectorGroup, App } from "ratio-core";
import { coco } from "../coco";

export class TypeElement extends Component<coco.taxonomy.Type, HTMLLIElement> implements coco.taxonomy.TypeListener, Selector {

  private group: SelectorGroup;
  private a: HTMLAnchorElement;

  constructor(group: SelectorGroup, type: coco.taxonomy.Type) {
    super(type, document.createElement('li'));
    this.group = group;
    this.element.classList.add('nav-item', 'list-group-item');

    this.a = document.createElement('a');
    this.a.classList.add('nav-link');
    this.a.href = '#';
    this.a.textContent = ' ' + type.get_name();
    this.a.addEventListener('click', (event) => {
      event.preventDefault();
      this.select_component();
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

  select(): void { this.a.classList.add('active'); }
  unselect(): void { this.a.classList.remove('active'); }

  select_component(): void { App.get_instance().selected_component(new Type(this.payload)); }
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

  private sp_body: HTMLTableSectionElement;
  private dp_body: HTMLTableSectionElement;

  constructor(type: coco.taxonomy.Type) {
    super(type, document.createElement('div'));
    this.element.classList.add('d-flex', 'flex-column', 'flex-grow-1');

    const sp_table = document.createElement('table');
    sp_table.classList.add('table');

    const sthead = sp_table.createTHead();
    const shrow = sthead.insertRow();
    const sp_name = shrow.insertCell();
    sp_name.scope = 'col';
    sp_name.textContent = 'Name';
    const sp_type = shrow.insertCell();
    sp_type.scope = 'col';
    sp_type.textContent = 'Type';
    this.sp_body = sp_table.createTBody();

    const dp_table = document.createElement('table');
    dp_table.classList.add('table');

    const dthead = sp_table.createTHead();
    const dhrow = dthead.insertRow();
    const dp_name = dhrow.insertCell();
    dp_name.scope = 'col';
    dp_name.textContent = 'Name';
    const dp_type = dhrow.insertCell();
    dp_type.scope = 'col';
    dp_type.textContent = 'Type';
    this.dp_body = dp_table.createTBody();

    this.set_static_properties();
    this.set_dynamic_properties();

    this.element.append(sp_table);
    this.element.append(dp_table);

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
    if (sps) {
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
    }
  }

  private set_dynamic_properties() {
    this.dp_body.innerHTML = '';
    const dps = this.payload.get_dynamic_properties();
    if (dps) {
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
    }
  }
}


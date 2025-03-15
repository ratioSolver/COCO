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

  private static_properties_table: HTMLTableElement;
  private dynamic_properties_table: HTMLTableElement;

  constructor(type: coco.taxonomy.Type) {
    super(type, document.createElement('div'));
    this.element.classList.add('d-flex', 'flex-column', 'flex-grow-1');

    this.static_properties_table = document.createElement('table');
    this.static_properties_table.classList.add('table');

    const sthead = this.static_properties_table.createTHead();
    const shrow = sthead.insertRow();
    const sproperty_name = shrow.insertCell();
    sproperty_name.scope = 'col';
    sproperty_name.textContent = 'Name';
    const sproperty_type = shrow.insertCell();
    sproperty_type.scope = 'col';
    sproperty_type.textContent = 'Type';
    this.static_properties_table.createTBody();

    this.dynamic_properties_table = document.createElement('table');
    this.dynamic_properties_table.classList.add('table');

    const dthead = this.static_properties_table.createTHead();
    const dhrow = dthead.insertRow();
    const dproperty_name = dhrow.insertCell();
    dproperty_name.scope = 'col';
    dproperty_name.textContent = 'Name';
    const dproperty_type = dhrow.insertCell();
    dproperty_type.scope = 'col';
    dproperty_type.textContent = 'Type';
    this.dynamic_properties_table.createTBody();

    this.element.append(this.static_properties_table);
    this.element.append(this.dynamic_properties_table);

    type.add_type_listener(this);
  }

  override unmounting(): void { this.payload.remove_type_listener(this); }

  parents_updated(_: coco.taxonomy.Type): void { }
  data_updated(_: coco.taxonomy.Type): void { }
  static_properties_updated(_: coco.taxonomy.Type): void { }
  dynamic_properties_updated(_: coco.taxonomy.Type): void { }
}


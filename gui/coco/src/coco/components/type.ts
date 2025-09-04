import { UListComponent, SelectorGroup, ListItemComponent, PayloadComponent } from "@ratiosolver/flick";
import { coco } from "../coco";
import { library, icon } from '@fortawesome/fontawesome-svg-core'
import { faCopy, faCube } from '@fortawesome/free-solid-svg-icons'

library.add(faCopy, faCube);

export class TypeElement extends ListItemComponent<coco.taxonomy.Type> implements coco.taxonomy.TypeListener {

  constructor(group: SelectorGroup, type: coco.taxonomy.Type) {
    super(group, type, icon(faCube).node[0], type.get_name());
    this.payload.add_type_listener(this);
  }

  parents_updated(_: coco.taxonomy.Type): void { }
  data_updated(_: coco.taxonomy.Type): void { }
  static_properties_updated(_: coco.taxonomy.Type): void { }
  dynamic_properties_updated(_: coco.taxonomy.Type): void { }

  override unmounting(): void {
    super.unmounting();
    this.payload.remove_type_listener(this);
  }
}

export class TypeList extends UListComponent<coco.taxonomy.Type> implements coco.CoCoListener {

  private group: SelectorGroup;

  constructor(group: SelectorGroup = new SelectorGroup(), tps: coco.taxonomy.Type[] = []) {
    super(tps.map(tp => new TypeElement(group, tp)), (t0: coco.taxonomy.Type, t1: coco.taxonomy.Type) => t0.get_name() === t1.get_name() ? 0 : (t0.get_name() < t1.get_name() ? -1 : 1));
    this.group = group;
    this.node.classList.add('nav', 'nav-pills', 'list-group', 'flex-column');
    coco.CoCo.get_instance().add_coco_listener(this);
  }

  override unmounting(): void { coco.CoCo.get_instance().remove_coco_listener(this); }

  new_type(type: coco.taxonomy.Type): void { this.add_child(new TypeElement(this.group, type)); }
  new_item(_: coco.taxonomy.Item): void { }
  new_intent(_: coco.llm.Intent): void { }
  new_entity(_: coco.llm.Entity): void { }
  new_slot(_: coco.llm.Slot): void { }
}

export class Type extends PayloadComponent<HTMLDivElement, coco.taxonomy.Type> implements coco.taxonomy.TypeListener {

  private sp_table: HTMLTableElement;
  private sp_body: HTMLTableSectionElement;
  private dp_table: HTMLTableElement;
  private dp_body: HTMLTableSectionElement;

  constructor(type: coco.taxonomy.Type) {
    super(document.createElement('div'), type);
    this.node.classList.add('d-flex', 'flex-column', 'flex-grow-1');
    this.node.style.margin = '1em';

    const name_div = document.createElement('div');
    name_div.classList.add('input-group');
    const name_input = document.createElement('input');
    name_input.classList.add('form-control');
    name_input.type = 'text';
    name_input.placeholder = 'Type name';
    name_input.value = type.get_name();
    name_input.disabled = true;
    name_div.append(name_input);
    const name_button_div = document.createElement('div');
    name_button_div.classList.add('input-group-append');
    const name_button = document.createElement('button');
    name_button.classList.add('btn', 'btn-outline-secondary');
    name_button.type = 'button';
    name_button.title = 'Copy type name to clipboard';
    name_button.append(icon(faCopy).node[0]);
    name_button.title = 'Copy type name to clipboard';
    name_button.addEventListener('click', () => {
      navigator.clipboard.writeText(type.get_name());
    });
    name_button_div.append(name_button);
    name_div.append(name_button_div);
    this.node.append(name_div);

    this.sp_table = document.createElement('table');
    this.sp_table.createCaption().textContent = 'Static properties';
    this.sp_table.classList.add('table', 'caption-top');

    const sp_thead = this.sp_table.createTHead();
    const sp_hrow = sp_thead.insertRow();
    const sp_name = document.createElement('th');
    sp_name.scope = 'col';
    sp_name.textContent = 'Name';
    sp_name.classList.add('w-75');
    sp_hrow.appendChild(sp_name);
    const sp_type = document.createElement('th');
    sp_type.scope = 'col';
    sp_type.textContent = 'Type';
    sp_type.classList.add('w-25');
    sp_hrow.appendChild(sp_type);

    this.sp_body = this.sp_table.createTBody();
    this.node.append(this.sp_table);

    this.dp_table = document.createElement('table');
    this.dp_table.createCaption().textContent = 'Dynamic properties';
    this.dp_table.classList.add('table', 'caption-top');

    const dp_thead = this.dp_table.createTHead();
    const dp_hrow = dp_thead.insertRow();
    const dp_name = document.createElement('th');
    dp_name.scope = 'col';
    dp_name.textContent = 'Name';
    dp_name.classList.add('w-75');
    dp_hrow.appendChild(dp_name);
    const dp_type = document.createElement('th');
    dp_type.scope = 'col';
    dp_type.textContent = 'Type';
    dp_type.classList.add('w-25');
    dp_hrow.appendChild(dp_type);

    this.dp_body = this.dp_table.createTBody();
    this.node.append(this.dp_table);

    this.set_static_properties();
    this.set_dynamic_properties();

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
    } else
      this.sp_table.hidden = true;
  }

  private set_dynamic_properties() {
    this.dp_body.innerHTML = '';
    const dps = this.payload.get_dynamic_properties();
    if (dps && dps.size > 0) {
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
    } else
      this.dp_table.hidden = true;
  }
}


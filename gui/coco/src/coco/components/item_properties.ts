import { blink, Component } from "@ratiosolver/flick";
import { coco } from "../coco";

export class ItemProperties extends Component<coco.taxonomy.Item, HTMLDivElement> implements coco.taxonomy.ItemListener {

  private readonly p_values = new Map<string, HTMLTableCellElement>();

  constructor(item: coco.taxonomy.Item) {
    super(item, document.createElement('div'));

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
    this.set_properties();

    item.add_item_listener(this);
  }

  properties_updated(_: coco.taxonomy.Item): void { this.set_properties(); }
  values_updated(_: coco.taxonomy.Item): void { }
  new_value(_i: coco.taxonomy.Item, _v: coco.taxonomy.Datum): void { }
  slots_updated(_: coco.taxonomy.Item): void { }

  private set_properties() {
    const props = this.payload.get_type().get_all_static_properties();
    if (props.size > 0) {
      this.element.hidden = false;
      const ps = this.payload.get_properties();
      if (ps)
        for (const [name, val] of Object.entries(ps)) {
          const p_val = this.p_values.get(name)!;
          blink(p_val);
          p_val.textContent = props.get(name)!.get_type().to_string(val);
        }
    } else
      this.element.hidden = true;
  }
}

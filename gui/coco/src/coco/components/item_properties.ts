import { blink, PayloadComponent } from "@ratiosolver/flick";
import { coco } from "../coco";

export class ItemProperties extends PayloadComponent<HTMLDivElement, coco.taxonomy.Item> implements coco.taxonomy.ItemListener {

  private readonly p_values = new Map<string, HTMLTableCellElement>();

  constructor(item: coco.taxonomy.Item) {
    super(document.createElement('div'), item);

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
    const itm_props = item.get_properties();
    for (const [name, prop] of coco.taxonomy.get_static_properties(item)) {
      const row = p_body.insertRow();
      const p_name = document.createElement('th');
      p_name.scope = 'col';
      p_name.textContent = name;
      row.appendChild(p_name);
      const p_value = document.createElement('td');
      this.p_values.set(name, p_value);
      if (itm_props && itm_props[name])
        p_value.textContent = prop.get_type().to_string(itm_props[name]);
      row.appendChild(p_value);
    }

    this.node.append(p_table);
    this.set_properties();

    item.add_item_listener(this);
  }

  types_updated(_: coco.taxonomy.Item): void { }
  properties_updated(_: coco.taxonomy.Item): void { this.set_properties(); }
  values_updated(_: coco.taxonomy.Item): void { }
  new_value(_i: coco.taxonomy.Item, _v: coco.taxonomy.Datum): void { }
  slots_updated(_: coco.taxonomy.Item): void { }

  private set_properties() {
    const static_props = coco.taxonomy.get_static_properties(this.payload);
    if (static_props.size > 0) {
      this.node.hidden = false;
      const ps = this.payload.get_properties();
      if (ps)
        for (const [name, val] of Object.entries(ps)) {
          if (!static_props.has(name)) {
            console.warn(`Property "${name}" is not a static property of item's types.`);
            continue;
          }
          const p_val = this.p_values.get(name)!;
          blink(p_val);
          p_val.textContent = static_props.get(name)!.get_type().to_string(val);
        }
    } else
      this.node.hidden = true;
  }
}

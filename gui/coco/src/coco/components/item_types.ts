import { App, blink, PayloadComponent } from "@ratiosolver/flick";
import { coco } from "../coco";
import { Type } from "./type";

export class ItemTypes extends PayloadComponent<HTMLDivElement, coco.taxonomy.Item> implements coco.taxonomy.ItemListener {

  private readonly types: Map<string, HTMLSpanElement> = new Map();
  private readonly types_row: HTMLDivElement;

  constructor(item: coco.taxonomy.Item) {
    super(document.createElement('div'), item);

    this.node.classList.add('d-flex', 'flex-column');
    this.node.style.marginTop = '1em';

    const types_label = document.createElement('span');
    types_label.textContent = 'Types:';
    this.node.appendChild(types_label);

    this.types_row = document.createElement('div');
    this.types_row.classList.add('d-flex', 'align-items-center');
    this.node.appendChild(this.types_row);

    const item_types = item.get_types();
    for (const type of item_types)
      this.add_type(type);

    item.add_item_listener(this);
  }

  types_updated(_: coco.taxonomy.Item): void {
    // Remove deleted types
    for (const [tp_name, span] of this.types)
      if (!this.payload.get_types().has(coco.CoCo.get_instance().get_type(tp_name)!)) {
        this.types_row.removeChild(span);
        this.types.delete(tp_name);
      }

    // Add new types
    for (const type of this.payload.get_types())
      if (!this.types.has(type.get_name()))
        this.add_type(type);
  }
  properties_updated(_: coco.taxonomy.Item): void { }
  values_updated(_: coco.taxonomy.Item): void { }
  new_value(_i: coco.taxonomy.Item, _v: coco.taxonomy.Datum): void { }
  slots_updated(_: coco.taxonomy.Item): void { }

  private add_type(type: coco.taxonomy.Type) {
    const span = document.createElement('span');
    span.classList.add('badge', 'bg-primary', 'me-1');
    span.textContent = type.get_name();
    span.style.cursor = "pointer";
    span.addEventListener('click', () => App.get_instance().selected_component(new Type(type)));
    this.types_row.appendChild(span);
    this.types.set(type.get_name(), span);
    blink(span);
  }
}
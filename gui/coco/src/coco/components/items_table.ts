import { App, blink, PayloadComponent } from "@ratiosolver/flick";
import { coco } from "../coco";
import { Item } from "./item";

export class ItemsTable extends PayloadComponent<HTMLDivElement, coco.taxonomy.Item[]> implements coco.CoCoListener, coco.taxonomy.ItemListener {

  private readonly type: coco.taxonomy.Type;
  private readonly tbody: HTMLTableSectionElement;
  private readonly items: Map<string, HTMLTableRowElement> = new Map();
  private readonly static_props: Map<string, coco.taxonomy.Property<unknown>> | undefined

  constructor(type: coco.taxonomy.Type, items: coco.taxonomy.Item[] = Array.from(type.get_instances())) {
    super(document.createElement("div"), items);
    this.type = type;
    this.static_props = type.get_static_properties();

    const table = document.createElement("table");
    table.classList.add("table", "table-hover", 'caption-top');
    table.createCaption().textContent = 'Items';

    const thead = document.createElement("thead");
    const header_row = document.createElement("tr");
    const id_th = document.createElement("th");
    id_th.textContent = "ID";
    header_row.appendChild(id_th);
    const static_props = type.get_static_properties();
    if (static_props)
      for (const [prop_name, _] of static_props) {
        const th = document.createElement("th");
        th.textContent = prop_name;
        header_row.appendChild(th);
      }
    thead.appendChild(header_row);
    table.appendChild(thead);

    this.tbody = document.createElement("tbody");
    for (const item of items)
      this.add_item(item);
    table.appendChild(this.tbody);

    this.node.appendChild(table);
    coco.CoCo.get_instance().add_coco_listener(this);
  }

  new_type(_: coco.taxonomy.Type): void { }
  new_item(item: coco.taxonomy.Item): void {
    if (item.get_types().has(this.type)) {
      this.add_item(item);
      item.add_item_listener(this);
    }
  }

  types_updated(item: coco.taxonomy.Item): void {
    if (item.get_types().has(this.type)) {
      if (!this.items.has(item.get_id()))
        this.new_item(item);
    } else {
      const row = this.items.get(item.get_id());
      if (row) {
        this.tbody.removeChild(row);
        this.items.delete(item.get_id());
        item.remove_item_listener(this);
      }
    }
  }
  properties_updated(item: coco.taxonomy.Item): void {
    const row = this.items.get(item.get_id());
    if (row && this.static_props) {
      const item_props = item.get_properties();
      let col_idx = 1;
      for (const [prop_name, prop] of this.static_props) {
        const td = row.children.item(col_idx) as HTMLTableCellElement;
        if (item_props && item_props[prop_name] !== undefined)
          td.textContent = prop.get_type().to_string(item_props[prop_name]);
        else
          td.textContent = "";
        col_idx += 1;
      }
      blink(row);
    }
  }
  values_updated(_item: coco.taxonomy.Item): void { }
  new_value(_item: coco.taxonomy.Item, _v: coco.taxonomy.Datum): void { }

  private add_item(item: coco.taxonomy.Item) {
    const row = document.createElement("tr");
    row.style.cursor = "pointer";
    const id_td = document.createElement("td");
    id_td.textContent = item.get_id();
    row.appendChild(id_td);
    if (this.static_props)
      for (const [prop_name, prop] of this.static_props) {
        const td = document.createElement("td");
        const item_props = item.get_properties();
        if (item_props && item_props[prop_name] !== undefined)
          td.textContent = prop.get_type().to_string(item_props[prop_name]);
        else
          td.textContent = "";
        row.appendChild(td);
      }
    row.addEventListener("click", () => App.get_instance().selected_component(new Item(item)));
    this.items.set(item.get_id(), row);
    this.tbody.appendChild(row);
  }

  override unmounting(): void {
    super.unmounting();
    coco.CoCo.get_instance().remove_coco_listener(this);
  }
}
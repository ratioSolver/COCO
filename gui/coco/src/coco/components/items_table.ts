import { App, PayloadComponent } from "@ratiosolver/flick";
import { coco } from "../coco";
import { Item } from "./item";

export class ItemsTable extends PayloadComponent<HTMLDivElement, coco.taxonomy.Item[]> implements coco.CoCoListener {

  private readonly type: coco.taxonomy.Type;
  private readonly tbody: HTMLTableSectionElement;

  constructor(type: coco.taxonomy.Type, items: coco.taxonomy.Item[] = Array.from(type.get_instances())) {
    super(document.createElement("div"), items);
    this.type = type;

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
    if (item.get_types().has(this.type))
      this.add_item(item);
  }

  private add_item(item: coco.taxonomy.Item) {
    const static_props = this.type.get_static_properties();
    const row = document.createElement("tr");
    row.style.cursor = "pointer";
    const id_td = document.createElement("td");
    id_td.textContent = item.get_id();
    row.appendChild(id_td);
    if (static_props)
      for (const [prop_name, prop] of static_props) {
        const td = document.createElement("td");
        const item_props = item.get_properties();
        if (item_props && item_props[prop_name] !== undefined)
          td.textContent = prop.get_type().to_string(item_props[prop_name]);
        else
          td.textContent = "";
        row.appendChild(td);
      }
    row.addEventListener("click", () => App.get_instance().selected_component(new Item(item)));
    this.tbody.appendChild(row);
  }

  override unmounting(): void {
    super.unmounting();
    coco.CoCo.get_instance().remove_coco_listener(this);
  }
}
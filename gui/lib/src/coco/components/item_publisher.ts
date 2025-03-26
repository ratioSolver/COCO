import { blink, Component } from "ratio-core";
import { coco } from "../coco";
import { publisher } from "./publisher";

export class ItemPublisher extends Component<coco.taxonomy.Item, HTMLDivElement> implements coco.taxonomy.ItemListener {

  private readonly val: Record<string, unknown> = {};
  private readonly v_sel_all_check: HTMLInputElement;
  private readonly v_checks = new Map<string, HTMLInputElement>();
  private readonly v_values = new Map<string, publisher.Publisher<unknown>>();

  constructor(item: coco.taxonomy.Item) {
    super(item, document.createElement('div'));

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
    this.v_sel_all_check = document.createElement('input');
    this.v_sel_all_check.classList.add('form-check-input');
    this.v_sel_all_check.type = 'checkbox';
    this.v_sel_all_check.addEventListener('change', () => {
      const val = this.payload.get_datum();
      for (const [name, check] of this.v_checks) {
        check.checked = this.v_sel_all_check.checked;
        if (!this.v_sel_all_check.checked && val && val.data[name])
          this.v_values.get(name)!.set_value(val.data[name]);
      }
    });
    v_sel_all.appendChild(this.v_sel_all_check);
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
      v_sel_check.addEventListener('change', () => {
        if (this.v_checks.values().every(check => check.checked)) {
          this.v_sel_all_check.indeterminate = false;
          this.v_sel_all_check.checked = true;
        }
        else if (this.v_checks.values().every(check => !check.checked)) {
          this.v_sel_all_check.indeterminate = false;
          this.v_sel_all_check.checked = false;
          const val = this.payload.get_datum();
          if (val && val.data[name])
            this.v_values.get(name)!.set_value(val.data[name]);
        }
        else
          this.v_sel_all_check.indeterminate = true;
      });
      v_sel_check.classList.add('property-checkbox');
      this.v_checks.set(name, v_sel_check);
      v_sel.appendChild(v_sel_check);
      row.appendChild(v_sel);

      const v_value = document.createElement('td');
      const pub = publisher.PublisherManager.get_instance().get_publisher_generator(prop.get_type().get_name()).make_publisher(name, prop, this.val);
      this.v_values.set(name, pub);
      v_value.append(pub.get_element());
      row.appendChild(v_value);
    }

    this.element.append(v_table);
    this.set_value();
    this.v_sel_all_check.checked = true;
    this.v_sel_all_check.dispatchEvent(new Event('change'));

    const b_div = document.createElement('div');
    b_div.classList.add('d-flex', 'justify-content-end', 'gap-2');
    const fake_button = document.createElement('button');
    fake_button.classList.add('btn', 'btn-secondary');
    fake_button.textContent = 'Fake';
    fake_button.addEventListener('click', () => {
      if (!this.v_sel_all_check.checked) {
        const pars: string[] = [];
        for (const [name, check] of this.v_checks)
          if (check.checked)
            pars.push(name);
        coco.CoCo.get_instance().fake_data(item.get_type(), pars).then(fake => {
          for (const [name, v] of Object.entries(fake)) {
            const v_val = this.v_values.get(name)!;
            blink(v_val.get_element().children[0] as HTMLElement, 500);
            v_val.set_value(v);
          }
        });
      } else
        coco.CoCo.get_instance().fake_data(item.get_type()).then(fake => {
          for (const [name, v] of Object.entries(fake)) {
            const v_val = this.v_values.get(name)!;
            blink(v_val.get_element().children[0] as HTMLElement, 500);
            v_val.set_value(v);
          }
        });
    });
    b_div.append(fake_button);
    const publish_button = document.createElement('button');
    publish_button.classList.add('btn', 'btn-primary');
    publish_button.textContent = 'Publish';
    publish_button.addEventListener('click', () => {
      const val: Record<string, unknown> = {};
      for (const [name, v] of Object.entries(this.val))
        if (this.v_checks.get(name)!.checked)
          val[name] = v;
      coco.CoCo.get_instance().publish(item, val);
    });
    b_div.append(publish_button);
    this.element.append(b_div);

    item.add_item_listener(this);
  }

  override unmounting(): void { this.payload.remove_item_listener(this); }

  properties_updated(_: coco.taxonomy.Item): void { }
  values_updated(_: coco.taxonomy.Item): void { }
  new_value(_i: coco.taxonomy.Item, _v: coco.taxonomy.Datum): void { this.set_value(); }

  private set_value() {
    const props = this.payload.get_type().get_all_dynamic_properties();
    if (props.size > 0) {
      this.element.hidden = false;
      const val = this.payload.get_datum();
      if (val)
        for (const [name, v] of Object.entries(val.data)) {
          const v_val = this.v_values.get(name)!;
          blink(v_val.get_element().children[0] as HTMLElement, 500);
          if (!this.v_checks.get(name)!.checked)
            v_val.set_value(v);
        }
    } else
      this.element.hidden = true;
  }
}
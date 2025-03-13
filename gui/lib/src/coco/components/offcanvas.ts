import { App, Component } from "ratio-core";
import { TypeList } from "./type_list";
import { ItemList } from "./item_list";

class OffcanvasBody extends Component<App, HTMLDivElement> {

  constructor(type_list: TypeList, item_list: ItemList) {
    super(App.get_instance(), document.createElement('div'));
    this.element.classList.add('offcanvas-body', 'flex-column', 'flex-shrink-0', 'p-3', 'bg-light');

    const types_lab = document.createElement('label');
    types_lab.innerText = "Types";
    this.element.append(types_lab);
    this.add_child(type_list);

    const items_lab = document.createElement('label');
    items_lab.innerText = "Items";
    this.element.append(items_lab);
    this.add_child(item_list);
  }
}

export class Offcanvas extends Component<App, HTMLDivElement> {

  private body: OffcanvasBody;

  constructor(id: string = 'coco-offcanvas', type_list: TypeList = new TypeList(), item_list: ItemList = new ItemList()) {
    super(App.get_instance(), document.createElement('div'));

    this.element.classList.add('offcanvas', 'offcanvas-start', 'd-flex');
    this.element.tabIndex = -1;
    this.element.id = id;

    this.body = new OffcanvasBody(type_list, item_list);

    this.add_child(this.body);
  }

  get_id(): string { return this.element.id; }
}
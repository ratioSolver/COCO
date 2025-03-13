import { App, Component } from "ratio-core";
import { TypeList } from "./type_list";
import { ItemList } from "./item_list";

class Body extends Component<App, HTMLDivElement> {

  constructor(type_list: TypeList, item_list: ItemList) {
    super(App.get_instance(), document.createElement('div'));
    this.element.classList.add('offcanvas-body', 'flex-column', 'flex-shrink-0', 'p-3', 'bg-light');

    this.add_child(type_list);
    this.add_child(item_list);
  }
}

export class Offcanvas extends Component<App, HTMLDivElement> {

  private body: Body;

  constructor(id: string, type_list: TypeList, item_list: ItemList) {
    super(App.get_instance(), document.createElement('div'));

    this.element.classList.add('offcanvas', 'offcanvas-start', 'd-flex');
    this.element.tabIndex = -1;
    this.element.id = id;

    this.body = new Body(type_list, item_list);

    this.add_child(this.body);
  }
}
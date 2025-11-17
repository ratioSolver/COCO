import { Component, SelectorGroup } from "@ratiosolver/flick";
import { TypeList } from "./type";
import { ItemList } from "./item";
import { TaxonomyElement } from "./taxonomy";

class ULComponent extends Component<HTMLUListElement> {

  constructor(group: SelectorGroup) {
    super(document.createElement('ul'));
    this.node.classList.add('nav', 'nav-pills', 'list-group', 'flex-column');

    this.add_child(new TaxonomyElement(group));
  }
}

class OffcanvasBody extends Component<HTMLDivElement> {

  private group = new SelectorGroup();
  private ul = new ULComponent(this.group);
  private type_list = new TypeList(this.group);
  private item_list = new ItemList(this.group);

  constructor() {
    super(document.createElement('div'));
    this.node.classList.add('offcanvas-body', 'flex-column', 'flex-shrink-0', 'p-3', 'bg-light');

    this.add_child(this.ul);

    const types_lab = document.createElement('label');
    types_lab.innerText = "Types";
    this.node.append(types_lab);
    this.add_child(this.type_list);

    const items_lab = document.createElement('label');
    items_lab.innerText = "Items";
    this.node.append(items_lab);
    this.add_child(this.item_list);
  }
}

export class Offcanvas extends Component<HTMLDivElement> {

  private body: OffcanvasBody;

  constructor(id: string = 'coco-offcanvas') {
    super(document.createElement('div'));

    this.node.classList.add('offcanvas', 'offcanvas-start', 'd-flex');
    this.node.tabIndex = -1;
    this.node.id = id;

    this.body = new OffcanvasBody();

    this.add_child(this.body);
  }

  get_id(): string { return this.node.id; }
}
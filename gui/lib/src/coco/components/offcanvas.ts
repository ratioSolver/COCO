import { App, Component, Selector, SelectorGroup } from "ratio-core";
import { TypeList } from "./type";
import { ItemList } from "./item";
import { TaxonomyGraph } from "./taxonomy";

class ULComponent extends Component<void, HTMLUListElement> {

  constructor(group: SelectorGroup) {
    super(undefined, document.createElement('ul'));
    this.element.classList.add('nav', 'nav-pills', 'list-group', 'flex-column');

    this.add_child(new TaxonomyElement(group));
  }
}

class TaxonomyElement extends Component<App, HTMLLIElement> implements Selector {

  private group: SelectorGroup;
  private a: HTMLAnchorElement;

  constructor(group: SelectorGroup) {
    super(App.get_instance(), document.createElement('li'));
    this.group = group;
    this.element.classList.add('nav-item', 'list-group-item');

    this.a = document.createElement('a');
    this.a.classList.add('nav-link');
    this.a.href = '#';
    this.a.textContent = ' Taxonomy';
    this.a.addEventListener('click', (event) => {
      event.preventDefault();
      App.get_instance().selected_component(new TaxonomyGraph());
      group.set_selected(this);
    });

    this.element.append(this.a);
    group.add_selector(this);
  }

  override unmounting(): void { this.group.remove_selector(this); }

  select(): void { this.a.classList.add('active'); }
  unselect(): void { this.a.classList.remove('active'); }
}

class OffcanvasBody extends Component<App, HTMLDivElement> {

  private group = new SelectorGroup();
  private ul = new ULComponent(this.group);
  private type_list = new TypeList(this.group);
  private item_list = new ItemList(this.group);

  constructor() {
    super(App.get_instance(), document.createElement('div'));
    this.element.classList.add('offcanvas-body', 'flex-column', 'flex-shrink-0', 'p-3', 'bg-light');

    this.add_child(this.ul);

    const types_lab = document.createElement('label');
    types_lab.innerText = "Types";
    this.element.append(types_lab);
    this.add_child(this.type_list);

    const items_lab = document.createElement('label');
    items_lab.innerText = "Items";
    this.element.append(items_lab);
    this.add_child(this.item_list);
  }
}

export class Offcanvas extends Component<App, HTMLDivElement> {

  private body: OffcanvasBody;

  constructor(id: string = 'coco-offcanvas') {
    super(App.get_instance(), document.createElement('div'));

    this.element.classList.add('offcanvas', 'offcanvas-start', 'd-flex');
    this.element.tabIndex = -1;
    this.element.id = id;

    this.body = new OffcanvasBody();

    this.add_child(this.body);
  }

  get_id(): string { return this.element.id; }
}
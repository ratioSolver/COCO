import { App, Component, Selector, SelectorGroup, Settings } from "@ratiosolver/flick";
import SwaggerUI from 'swagger-ui'
import 'swagger-ui/dist/swagger-ui.css';
import { library, icon } from '@fortawesome/fontawesome-svg-core'
import { faFileCode, faRightLeft } from '@fortawesome/free-solid-svg-icons'

library.add(faFileCode, faRightLeft);

export class APIElement extends Component<void, HTMLLIElement> implements Selector {

  private group: SelectorGroup;
  private a: HTMLAnchorElement;

  constructor(group: SelectorGroup) {
    super(undefined, document.createElement('li'));
    this.group = group;
    this.element.classList.add('nav-item', 'list-group-item');

    this.a = document.createElement('a');
    this.a.classList.add('nav-link', 'd-flex', 'align-items-center');
    this.a.href = '#';
    const icn = icon(faFileCode).node[0];
    icn.classList.add('me-2');
    this.a.append(icn);
    this.a.append(document.createTextNode('OpenAPI'));
    this.a.addEventListener('click', (event) => {
      event.preventDefault();
      group.set_selected(this);
    });

    this.element.append(this.a);
    group.add_selector(this);
  }

  override unmounting(): void { this.group.remove_selector(this); }

  select(): void {
    this.a.classList.add('active');
    App.get_instance().selected_component(new APIComponent(Settings.get_instance().get_host() + '/openapi'));
  }
  unselect(): void { this.a.classList.remove('active'); }
}

export class AsyncElement extends Component<void, HTMLLIElement> implements Selector {

  private group: SelectorGroup;
  private a: HTMLAnchorElement;

  constructor(group: SelectorGroup) {
    super(undefined, document.createElement('li'));
    this.group = group;
    this.element.classList.add('nav-item', 'list-group-item');

    this.a = document.createElement('a');
    this.a.classList.add('nav-link', 'd-flex', 'align-items-center');
    this.a.href = '#';
    const icn = icon(faRightLeft).node[0];
    icn.classList.add('me-2');
    this.a.append(icn);
    this.a.append(document.createTextNode('AsyncAPI'));
    this.a.addEventListener('click', (event) => {
      event.preventDefault();
      group.set_selected(this);
    });

    this.element.append(this.a);
    group.add_selector(this);
  }

  override unmounting(): void { this.group.remove_selector(this); }

  select(): void {
    this.a.classList.add('active');
    App.get_instance().selected_component(new APIComponent(Settings.get_instance().get_host() + '/asyncapi'));
  }
  unselect(): void { this.a.classList.remove('active'); }
}

export class APIComponent extends Component<void, HTMLDivElement> {

  readonly ui: SwaggerUI;

  constructor(url: string = Settings.get_instance().get_host() + '/openapi') {
    super(undefined, document.createElement('div'));
    this.ui = SwaggerUI({ domNode: this.element, url: url });
  }
}

import { Component, Settings } from "@ratiosolver/flick";
import SwaggerUI from 'swagger-ui'
import 'swagger-ui/dist/swagger-ui.css';

export class TypeElement extends Component<void, HTMLDivElement> {

  readonly ui: SwaggerUI;

  constructor(url: string = Settings.get_instance().get_host() + '/openapi') {
    super(void 0, document.createElement('div'));
    this.ui = SwaggerUI({ domNode: this.element, url: url });
  }
}

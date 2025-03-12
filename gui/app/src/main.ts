import { AppComponent, Component, Offcanvas } from 'ratio-core';
import './styles.css'

const offcanvas_id = "CoCoCanvas";

class CoCoApp extends AppComponent {

  offcanvas: Offcanvas;

  constructor() {
    super();

    const top = new Map<HTMLAnchorElement, Component<any, HTMLElement>>();

    this.offcanvas = new Offcanvas(offcanvas_id, top);
    this.add_child(this.offcanvas);
  }

  populate_navbar(container: HTMLDivElement): void {
    const brand = document.createElement('a');
    brand.classList.add('navbar-brand');
    brand.text = " CoCo";
    brand.setAttribute('data-bs-toggle', 'offcanvas');
    brand.href = '#' + offcanvas_id;

    const brand_icon = document.createElement('img');
    brand_icon.src = 'favicon.ico';
    brand_icon.alt = 'CoCo';
    brand_icon.width = 32;
    brand_icon.height = 32;
    brand_icon.classList.add('d-inline-block', 'align-text-top', 'mr-2');
    brand.prepend(brand_icon);

    container.appendChild(brand);
  }
}

new CoCoApp();
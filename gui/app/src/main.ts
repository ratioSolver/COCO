import { AppComponent, Connection, Settings } from 'ratio-core';
import { coco, Offcanvas } from 'ratio-coco-lib';
import { Offcanvas as BootstrapOffcanvas } from 'bootstrap';
import './styles.css'

Settings.get_instance().load_settings({ hostname: location.host, port: 8080, ws_path: 'coco' });

const offcanvas_id = 'coco-offcanvas';

class CoCoApp extends AppComponent {

  offcanvas: Offcanvas;

  constructor() {
    super();

    this.offcanvas = new Offcanvas(offcanvas_id);
    this.add_child(this.offcanvas);

    Connection.get_instance().connect();
  }

  override populate_navbar(container: HTMLDivElement): void {
    // Create and add brand element
    const brand = document.createElement('a');
    brand.classList.add('navbar-brand');
    brand.href = '#';
    brand.addEventListener('click', (event) => {
      event.preventDefault();
      const offcanvasElement = document.getElementById(offcanvas_id);
      if (offcanvasElement) {
        BootstrapOffcanvas.getOrCreateInstance(offcanvasElement).show();
      }
    });

    const brand_container = document.createElement('div');
    brand_container.style.display = 'flex';
    brand_container.style.alignItems = 'center';
    brand_container.style.gap = '0.5rem'; // Add space between icon and text

    const brand_icon = document.createElement('img');
    brand_icon.src = 'favicon.ico';
    brand_icon.alt = 'CoCo';
    brand_icon.width = 32;
    brand_icon.height = 32;
    brand_icon.classList.add('d-inline-block', 'align-text-top', 'me-1');

    const brand_text = document.createElement('span');
    brand_text.textContent = 'CoCo';
    brand_text.style.fontWeight = '500'; // Make text slightly bolder

    brand_container.appendChild(brand_icon);
    brand_container.appendChild(brand_text);
    brand.appendChild(brand_container);

    container.appendChild(brand);
  }

  override received_message(message: any): void { coco.CoCo.get_instance().update_coco(message); }
}

new CoCoApp();
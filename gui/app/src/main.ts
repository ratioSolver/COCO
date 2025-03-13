import { AppComponent, Connection, Settings } from 'ratio-core';
import { coco, Offcanvas, TaxonomyGraph } from 'coco-lib';
import './styles.css'

Settings.get_instance().load_settings({ hostname: location.host, port: 8080, ws_path: 'coco' });

const offcanvas_id = 'coco-offcanvas';

class CoCoApp extends AppComponent {

  offcanvas: Offcanvas;
  taxonomy_graph: TaxonomyGraph;

  constructor() {
    super();

    this.offcanvas = new Offcanvas(offcanvas_id);
    this.add_child(this.offcanvas);

    this.taxonomy_graph = new TaxonomyGraph();
    this.add_child(this.taxonomy_graph);

    Connection.get_instance().connect();
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

  override received_message(message: any): void { coco.CoCo.get_instance().update_coco(message); }
}

new CoCoApp();
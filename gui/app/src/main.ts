import { AppComponent, BrandComponent, Connection, LogInButton, LogInModal, LogOutButton, NavbarContent, NewUserButton, NewUserModal, Settings } from '@ratiosolver/flick';
import { coco, Offcanvas as CoCoOffcanvas } from '@ratiosolver/coco';
import './styles.css'
import { Offcanvas } from 'bootstrap';

Settings.get_instance().load_settings({ ws_path: '/coco' });

const has_users = false;
const offcanvas_id = 'coco-offcanvas';

class CoCoBrandComponent extends BrandComponent {

  constructor() {
    super('CoCo', 'favicon.ico', 32, 32);
    this.node.id = 'coco-brand';
  }
}

class CoCoNavbarContent extends NavbarContent {
  constructor() {
    super();

    if (has_users) {
      this.add_child(new NewUserButton());
      this.add_child(new LogInButton());
      this.add_child(new LogOutButton());
    }
  }
}

class CoCoApp extends AppComponent {

  constructor() {
    super(new CoCoBrandComponent(), new CoCoNavbarContent());

    const offcanvas = new CoCoOffcanvas(offcanvas_id);
    this.add_child(offcanvas);
    document.getElementById('coco-brand')?.addEventListener('click', (event) => {
      event.preventDefault();
      Offcanvas.getOrCreateInstance(document.getElementById(offcanvas_id)!).toggle();
    });

    if (has_users) {
      this.add_child(new NewUserModal());
      this.add_child(new LogInModal());
      const token = localStorage.getItem('token');
      if (token)
        Connection.get_instance().connect(token);
    }
    else
      Connection.get_instance().connect();
  }

  override received_message(message: any): void { coco.CoCo.get_instance().update_coco(message); }

  override connection_error(_error: any): void { this.toast('Connection error. Please try again later.'); }
}

new CoCoApp();
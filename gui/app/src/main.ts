import { AppComponent, BrandComponent, Connection, LogInButton, LogInModal, LogOutButton, NewUserButton, NewUserModal, Settings } from '@ratiosolver/flick';
import { coco, Offcanvas } from '@ratiosolver/coco';
import './styles.css'

Settings.get_instance().load_settings({ ws_path: '/coco' });

const has_users = true;
const offcanvas_id = 'coco-offcanvas';

class CoCoApp extends AppComponent {

  constructor() {
    super();

    // Create and add brand element
    this.navbar.add_child(new BrandComponent('CoCo', 'favicon.ico', 32, 32, offcanvas_id));

    this.add_child(new Offcanvas(offcanvas_id));

    if (has_users) {
      this.navbar.add_child(new NewUserButton());
      this.navbar.add_child(new LogInButton());
      this.navbar.add_child(new LogOutButton());
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
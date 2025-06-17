import { AppComponent, Connection, LogInButton, LogInModal, LogOutButton, NewUserButton, NewUserModal, Settings } from '@ratiosolver/flick';
import { coco, Offcanvas } from '@ratiosolver/coco';
import { Offcanvas as BootstrapOffcanvas } from 'bootstrap';
import './styles.css'

Settings.get_instance().load_settings({ ws_path: '/coco' });

const has_users = true;
const offcanvas_id = 'coco-offcanvas';

class CoCoApp extends AppComponent {

  private readonly offcanvas = new Offcanvas(offcanvas_id);
  private new_user_button: NewUserButton | null = null;
  private readonly new_user_modal = new NewUserModal();
  private log_in_button: LogInButton | null = null;
  private readonly log_in_modal = new LogInModal();
  private log_out_button: LogOutButton | null = null;

  constructor() {
    super();

    this.add_child(this.offcanvas);

    if (has_users) {
      this.add_child(this.new_user_modal);
      this.add_child(this.log_in_modal);
    }
    else
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

    if (has_users) {
      this.new_user_button = new NewUserButton();
      this.new_user_button.element.style.marginLeft = 'auto';
      this.new_user_button.element.style.display = 'inline-block';
      container.appendChild(this.new_user_button.element);

      this.log_in_button = new LogInButton();
      this.log_in_button.element.style.marginLeft = '10px';
      this.log_in_button.element.style.display = 'inline-block';
      container.appendChild(this.log_in_button.element);

      this.log_out_button = new LogOutButton();
      this.log_out_button.element.style.marginLeft = '10px';
      this.log_out_button.element.style.display = 'none'; // Initially hidden
      container.appendChild(this.log_out_button.element);
    }
  }

  override logged_in(_info: any): void {
    console.log('User logged in');
    if (has_users) {
      this.new_user_button!.element.style.display = 'none';
      this.log_in_button!.element.style.display = 'none';
      this.log_out_button!.element.style.display = 'inline-block';
    }
  }

  override logged_out(): void {
    console.log('User logged out');
    if (has_users) {
      this.new_user_button!.element.style.display = 'inline-block';
      this.log_in_button!.element.style.display = 'inline-block';
      this.log_out_button!.element.style.display = 'none';
    }
  }

  override received_message(message: any): void { coco.CoCo.get_instance().update_coco(message); }

  override connection_error(_error: any): void { this.toast('Connection error. Please try again later.'); }
}

new CoCoApp();
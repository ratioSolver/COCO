import { AppComponent } from 'ratio-core';
import './styles.css'

class CoCoApp extends AppComponent {

  populate_navbar(container: HTMLDivElement): void {
    const brand = document.createElement('a');
    brand.classList.add('navbar-brand');
    brand.text = " CoCo";

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
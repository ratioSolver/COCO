import { App, flick, Navbar, NavbarItem, NavbarList, OffcanvasBrand } from '@ratiosolver/flick';
import { h } from 'snabbdom';
import { CoCo } from 'coco';

const coco = new CoCo();
coco.connect();

// 3. Mount the application
flick.mount(() => {
  // The main render function returns the entire app view

  const navbar = Navbar(
    OffcanvasBrand('Counter App'),
    NavbarList([
      NavbarItem('Home', () => console.log('Navigating to Home'), true)
    ])
  );

  const content = h('div.container.mt-5.text-center', [
    h('h1', 'Welcome to Flick')
  ]);

  // The App component typically wraps the layout
  return App(navbar, content);
});

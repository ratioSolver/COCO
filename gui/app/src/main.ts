import { App, flick, Navbar, OffcanvasBrand } from '@ratiosolver/flick';
import { h } from 'snabbdom';
import { coco } from 'coco';

const cc = new coco.CoCo();
cc.connect();

// 3. Mount the application
flick.mount(() => {
  // The main render function returns the entire app view

  const navbar = Navbar(
    OffcanvasBrand('Counter App')
  );

  const content = h('div.container.mt-5.text-center', [
    h('h1', 'Welcome to CoCo'),
  ]);

  // The App component typically wraps the layout
  return App(navbar, content);
});

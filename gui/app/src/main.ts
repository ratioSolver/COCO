import { App, flick, Navbar, OffcanvasBrand } from '@ratiosolver/flick';
import { h } from 'snabbdom';
import { coco, Offcanvas } from 'coco';

const cc = new coco.CoCo();
cc.connect();

// 3. Mount the application
flick.mount(() => {
  // The main render function returns the entire app view

  const navbar = Navbar(
    OffcanvasBrand('Counter App')
  );

  const content = h('div.container.mt-5.text-center', [
    flick.ctx.current_page || h('h1', 'Welcome to CoCo'),
    Offcanvas(cc)
  ]);

  // The App component typically wraps the layout
  return App(navbar, content);
});

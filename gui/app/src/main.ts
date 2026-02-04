import { App, flick, Navbar, OffcanvasBrand } from '@ratiosolver/flick';
import { h } from 'snabbdom';
import { coco, Offcanvas } from 'coco';

const cc = new coco.CoCo();
cc.connect();

flick.mount(() => {
  const content = h('div', [
    flick.ctx.current_page || h('div.container.mt-5.text-center', h('h1', 'Welcome to CoCo')),
    Offcanvas(cc)
  ]);

  return App(Navbar(OffcanvasBrand('CoCo')), content);
});

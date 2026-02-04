import { App, flick, Navbar, OffcanvasBrand } from '@ratiosolver/flick';
import { h } from 'snabbdom';
import { coco, Offcanvas } from 'coco';

const cc = new coco.CoCo();
cc.connect();

flick.mount(() => {
  const content = h('div', [
    flick.ctx.current_page || h('div.container.mt-5', [
      h('div.text-center.mb-5', [
        h('h1.display-4', 'CoCo'),
        h('p.lead', 'Combined Deduction and Abduction Reasoner'),
      ]),
      h('div.row.justify-content-center', [
        h('div.col-lg-8', [
          h('p', 'CoCo is a dual-process inspired cognitive architecture built in Rust. It integrates a rule-based expert system and a timeline-based planner to invoke deductive and abductive reasoning in dynamic environments.'),
          h('hr.my-4'),
          h('h4', 'Features'),
          h('ul.list-group.list-group-flush', [
            h('li.list-group-item', [h('strong', 'Hybrid Reasoning'), ': Unites deductive logic with abductive inference.']),
            h('li.list-group-item', [h('strong', 'Rust Core'), ': Designed for performance, memory safety, and concurrency.']),
            h('li.list-group-item', [h('strong', 'CLIPS Integration'), ': Seamless binding with the C-based CLIPS expert system.']),
            h('li.list-group-item', [h('strong', 'Web Interface'), ': Includes a web server (Axum) and visualization tools.']),
          ])
        ])
      ])
    ]),
    Offcanvas(cc)
  ]);

  return App(Navbar(OffcanvasBrand('CoCo')), content);
});

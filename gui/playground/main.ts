import { App, flick, Navbar, OffcanvasBrand } from '@ratiosolver/flick';
import { h } from 'snabbdom';
import { coco } from '../src/coco';
import { CoCoOffcanvas } from '../src/components/offcanvas';
import '@fortawesome/fontawesome-free/css/all.css';

const cc = new coco.CoCo({ url: 'ws://localhost:3000/ws' });
cc.add_listener({
  initialized: () => flick.redraw(),
  created_class: (_cls) => flick.redraw(),
  created_object: (_obj) => flick.redraw(),
  created_rule: (_rule) => flick.redraw(),
  connection_error: (error) => console.error('CoCo connection error', error),
  connected: () => { },
  disconnected: () => { },
});

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
    CoCoOffcanvas(cc)
  ]);

  return App(Navbar(OffcanvasBrand('CoCo')), content);
});

cc.connect();
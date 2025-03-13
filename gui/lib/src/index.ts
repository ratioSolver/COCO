import { ComputePositionConfig } from '@floating-ui/dom';

declare module 'cytoscape-popper' {

  interface PopperOptions extends ComputePositionConfig {
  }

  interface PopperInstance {
    update(): void;
    destroy(): void;
  }
}

import cytoscape from 'cytoscape';
import dagre from 'cytoscape-dagre';
import cytoscapePopper, { PopperInstance, PopperOptions, RefElement } from 'cytoscape-popper';
import {
  computePosition,
  flip,
  shift,
  limitShift
} from '@floating-ui/dom';

// // @ts-ignore
// import cytoscapePopper from 'cytoscape-popper';

export { coco } from './coco/coco';
export { TaxonomyGraph } from './coco/components/taxonomy';
export { Offcanvas } from './coco/components/offcanvas';

function popperFactory(ref: RefElement, content: HTMLElement, options?: PopperOptions): PopperInstance {
  const popperOptions = { middleware: [flip(), shift({ limiter: limitShift() })], ...options, };

  function update() {
    computePosition(ref, content, popperOptions).then(({ x, y }) => {
      Object.assign(content.style, {
        left: `${x}px`,
        top: `${y}px`,
      });
    });
  }

  function destroy() {
    content.remove();
  }

  update();
  return { update, destroy };
}

cytoscape.use(dagre);
cytoscape.use(cytoscapePopper(popperFactory));
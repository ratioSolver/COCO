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

export { coco } from './coco/coco';
export * from './coco/components/api';
export * from './coco/components/taxonomy';
export * from './coco/components/map';
export * from './coco/components/offcanvas';
export * from './coco/components/type'
export * from './coco/components/item'

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
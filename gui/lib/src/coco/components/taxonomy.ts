import { Component } from "ratio-core";
import { coco } from "../coco";
import cytoscape from 'cytoscape';

export class TaxonomyGraph extends Component<coco.CoCo, HTMLDivElement> implements coco.CoCoListener, coco.taxonomy.TypeListener {

  private cy: cytoscape.Core | null = null;
  private layout = {
    name: 'dagre',
    rankDir: 'BT',
    fit: false,
    nodeDimensionsIncludeLabels: true
  };
  private tooltip_style = "position: absolute; top: 0; left: 0; background-color: #444; color: white; border-radius: 4px; opacity: 0.8;";

  constructor(id: string = 'taxonomy-graph') {
    super(coco.CoCo.get_instance(), document.createElement('div'));
    this.element.id = id;
    this.element.classList.add('d-flex', 'flex-column', 'flex-grow-1');
  }

  override mounted(): void {
    this.cy = cytoscape({
      container: this.element,
      layout: this.layout,
      style: [
        {
          selector: 'node',
          style: {
            'shape': 'ellipse',
            'label': 'data(id)',
            'border-width': '1px',
            'border-color': '#666',
            'background-color': '#FFD700',
          }
        },
        {
          selector: 'edge[type="is_a"]',
          style: {
            'curve-style': 'bezier',
            'line-color': '#666',
            'target-arrow-color': '#666',
            'target-arrow-shape': 'triangle',
            'width': '1px'
          }
        },
        {
          selector: 'edge[type="static_property"]',
          style: {
            'curve-style': 'bezier',
            'label': 'data(name)',
            'line-color': '#666',
            'target-arrow-color': '#666',
            'target-arrow-shape': 'diamond',
            'width': '1px',
            'line-style': 'dashed'
          }
        },
        {
          selector: 'edge[type="dynamic_property"]',
          style: {
            'curve-style': 'bezier',
            'label': 'data(name)',
            'line-color': '#666',
            'target-arrow-color': '#666',
            'target-arrow-shape': 'diamond',
            'width': '1px',
            'line-style': 'dotted'
          }
        }
      ]
    });

    for (const tp of this.payload.get_types())
      this.create_type_node(tp);
    this.cy.layout(this.layout).run();

    this.payload.add_coco_listener(this);
  }

  override unmounting(): void {
    for (const tp of this.payload.get_types())
      tp.remove_type_listener(this);
    this.payload.remove_coco_listener(this);
  }

  new_type(type: coco.taxonomy.Type): void {
    this.create_type_node(type);
    this.cy!.layout(this.layout).run();
    type.add_type_listener(this);
  }

  parents_updated(type: coco.taxonomy.Type): void {
    this.cy!.elements(`edge[id ^= "p-${type.get_name()}"]`).remove();
    const pars = type.get_parents();
    if (pars)
      for (const par of pars)
        this.cy!.add({ group: 'edges', data: { id: `p-${type.get_name()}-${par.get_name()}`, type: 'is_a', source: type.get_name(), target: par.get_name() } });
  }
  data_updated(_: coco.taxonomy.Type): void { }
  static_properties_updated(type: coco.taxonomy.Type): void {
    this.cy!.elements(`edge[id ^= "sp-${type.get_name()}"]`).remove();
    const static_props = type.get_static_properties();
    if (static_props)
      for (const [name, prop] of static_props)
        if (prop instanceof coco.taxonomy.ItemProperty)
          this.cy!.add({ group: 'edges', data: { id: `sp-${type.get_name()}-${prop.get_domain().get_name()}`, type: 'static_property', name: name, source: type.get_name(), target: prop.get_domain().get_name() } });
  }
  dynamic_properties_updated(type: coco.taxonomy.Type): void {
    this.cy!.elements(`edge[id ^= "dp-${type.get_name()}"]`).remove();
    const dynamic_props = type.get_static_properties();
    if (dynamic_props)
      for (const [name, prop] of dynamic_props)
        if (prop instanceof coco.taxonomy.ItemProperty)
          this.cy!.add({ group: 'edges', data: { id: `dp-${type.get_name()}-${prop.get_domain().get_name()}`, type: 'dynamic_property', name: name, source: type.get_name(), target: prop.get_domain().get_name() } });
  }

  new_item(_: coco.taxonomy.Item): void { }

  private create_type_node(type: coco.taxonomy.Type): cytoscape.CollectionReturnValue {
    const tn = this.cy!.add({ group: 'nodes', data: { id: type.get_name() } });
    tn.on('mouseover', () => {
      const popper = tn.popper({
        content: () => {
          var div = document.createElement('div');
          div.style.cssText = this.tooltip_style;
          div.innerHTML = type.to_string();
          document.body.appendChild(div);
          return div;
        }
      });
      tn.scratch('popper', popper);
    });
    tn.on('mouseout', () => {
      const popper = tn.scratch('popper');
      if (popper) {
        popper.destroy();
        tn.removeScratch('popper');
      }
    });
    tn.on('position', () => {
      const popper = tn.scratch('popper');
      if (popper)
        popper.update();
    });

    return tn;
  }
}

import 'leaflet/dist/leaflet.css';
import * as L from 'leaflet';
import { App, Component, Selector, SelectorGroup } from '@ratiosolver/flick';
import { library, icon } from '@fortawesome/fontawesome-svg-core'
import { faMap } from '@fortawesome/free-solid-svg-icons'

library.add(faMap);

export class MapElement extends Component<void, HTMLLIElement> implements Selector {

  private group: SelectorGroup;
  private a: HTMLAnchorElement;
  private map_factory: () => MapComponent;

  constructor(group: SelectorGroup, map_factory: () => MapComponent = () => new MapComponent()) {
    super(undefined, document.createElement('li'));
    this.group = group;
    this.map_factory = map_factory;
    this.element.classList.add('nav-item', 'list-group-item');

    this.a = document.createElement('a');
    this.a.classList.add('nav-link', 'd-flex', 'align-items-center');
    this.a.href = '#';
    const icn = icon(faMap).node[0];
    icn.classList.add('me-2');
    this.a.append(icn);
    this.a.append(document.createTextNode('OpenAPI'));
    this.a.addEventListener('click', (event) => {
      event.preventDefault();
      group.set_selected(this);
    });

    this.element.append(this.a);
    group.add_selector(this);
  }

  override unmounting(): void { this.group.remove_selector(this); }

  select(): void {
    this.a.classList.add('active');
    App.get_instance().selected_component(this.map_factory());
  }
  unselect(): void { this.a.classList.remove('active'); }
}

export interface Layer {

  add_to(map: L.Map): void;
  remove_from(map: L.Map): void;
}

export class MapLayer<P = any> implements Layer {

  protected layer = L.layerGroup<P>();

  add_to(map: L.Map): void { this.layer.addTo(map); }
  remove_from(map: L.Map): void { this.layer.removeFrom(map); }
}

export class MapComponent extends Component<void, HTMLDivElement> {

  private map: L.Map | undefined;

  constructor() {
    super(undefined, document.createElement('div'));
    this.element.style.width = '100%';
    this.element.style.height = '100%';
  }

  get_map(): L.Map { return this.map!; }

  override mounted(): void {
    this.map = L.map(this.element);

    // Set initial view to Rome, Italy
    this.set_view([41.9028, 12.4964], 13);
  }

  /**
   * Adds a tile layer to the map using the specified URL and options.
   *
   * @param url - The URL template for the tile layer. Defaults to OpenStreetMap's standard tile server.
   * @param options - Optional configuration options for the tile layer.
   */
  add_tile_layer(url: string = 'https://{s}.tile.openstreetmap.org/{z}/{x}/{y}.png', options: L.TileLayerOptions = {}): void {
    L.tileLayer(url, options).addTo(this.map!);
  }

  /**
   * Sets the view of the map to the specified center and zoom level.
   *
   * @param center - The geographical center of the map view, specified as a latitude and longitude pair.
   * @param zoom - The zoom level for the map view. Optional; if not provided, defaults to the current zoom level.
   * @param options - Optional parameters for zooming and panning.
   */
  set_view(center: L.LatLngExpression, zoom?: number, options?: L.ZoomPanOptions): this {
    this.map!.setView(center, zoom, options);
    return this;
  }

  /**
   * Adds the specified layer to the map instance.
   *
   * @param layer - The layer to be added to the map.
   */
  add_layer(layer: Layer): void { layer.add_to(this.map!); }
  /**
   * Removes the specified layer from the map instance.
   *
   * @param layer - The layer to be removed from the map.
   */
  remove_layer(layer: Layer): void { layer.remove_from(this.map!); }
}

export interface HeatTile {
  bounds: L.LatLngBoundsExpression;
  value: number;
}

export class HeatMapLayer extends MapLayer<HeatTile> {

  constructor(tiles: HeatTile[] = []) {
    super();
    this.add_tiles(tiles);
  }

  add_tiles(tiles: HeatTile[]): void {
    const min = Math.min(...tiles.map(tile => tile.value));
    const max = Math.max(...tiles.map(tile => tile.value));

    const get_color = (value: number): string => {
      const hue = (value - min) / (max - min);
      return `hsl(${120 - hue * 120}, 100%, 50%)`;
    }

    tiles.forEach(tile => {
      const rect = L.rectangle(tile.bounds, { color: undefined, fillColor: get_color(tile.value), weight: 1, fillOpacity: 0.5 });
      this.layer.addLayer(rect);
    });
  }
}

export class GeoJSONLayer extends MapLayer {

  constructor(geojson: GeoJSON.FeatureCollection | GeoJSON.Feature) {
    super();
    this.add_geojson(geojson);
  }

  add_geojson(geojson: GeoJSON.FeatureCollection | GeoJSON.Feature): void {
    const layer = L.geoJSON(geojson);
    this.layer.addLayer(layer);
  }
}
import 'leaflet/dist/leaflet.css';
import * as L from 'leaflet';
import { Component, Selector, SelectorGroup, UListElement } from '@ratiosolver/flick';
import { library, icon } from '@fortawesome/fontawesome-svg-core'
import { faMap } from '@fortawesome/free-solid-svg-icons'
import { coco } from '../coco';

library.add(faMap);

export class MapElement extends UListElement<void> implements Selector {

  /**
   * Constructs a new instance of the class.
   *
   * @param group - The `SelectorGroup` instance to which this component belongs.
   * @param text - The label text for the map component. Defaults to `'Map'`.
   * @param map_factory - A factory function that returns a new `MapComponent` instance. Defaults to a function that creates a new `MapComponent`.
   */
  constructor(group: SelectorGroup, text: string = 'Map', map_factory: () => MapComponent = () => new MapComponent()) {
    super(group, undefined, icon(faMap).node[0], text, map_factory);
  }
}

export interface Layer {

  add_to(map: L.Map): void;
  remove_from(map: L.Map): void;

  unmount(): void;
}

export class MapLayer<P = any> implements Layer {

  protected layer = L.layerGroup<P>();

  add_to(map: L.Map): void { this.layer.addTo(map); }
  remove_from(map: L.Map): void { this.layer.removeFrom(map); }

  unmount(): void {
    this.layer.clearLayers();
    this.layer.remove();
  }
}

export class MapComponent extends Component<void, HTMLDivElement> {

  protected map: L.Map | undefined;

  constructor() {
    super(undefined, document.createElement('div'));
    this.element.style.width = '100%';
    this.element.style.height = '100%';
  }

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

export class CircleLayer<P> extends MapLayer<P> {

  private elements = new Map<P, L.CircleMarker<P>>();
  private latlng_factory: (element: P) => L.LatLngExpression;
  private radius_factory?: (element: P) => number;
  private color_factory?: (element: P) => string;
  private popup_factory?: (element: P) => string;

  constructor(latlng_factory: (element: P) => L.LatLngExpression, elements: P[] = []) {
    super();
    this.latlng_factory = latlng_factory;
    this.add_elements(elements);
  }

  /**
   * Adds elements to the layer, creating circle markers for each element.
   *
   * @param els - A single element of type `P` or an array of elements of type `P` to be added to the layer.
   */
  add_elements(els: P | P[]): void {
    if (!Array.isArray(els))
      els = [els];

    for (const el of els) {
      const circle = L.circleMarker<P>(this.latlng_factory(el), {
        radius: this.radius_factory ? this.radius_factory(el) : 5,
        color: this.color_factory ? this.color_factory(el) : '#3388ff',
      });

      if (this.popup_factory)
        circle.bindPopup(this.popup_factory(el));

      this.layer.addLayer(circle);
      this.elements.set(el, circle);
    }
  }

  /**
   * Checks if an element is already present in the layer.
   *
   * @param el - The element of type `P` to check for existence in the layer.
   * @returns A boolean indicating whether the element exists in the layer.
   */
  has_element(el: P): boolean { return this.elements.has(el); }

  /**
   * Updates the position, radius, color, and popup content of existing elements in the layer.
   *
   * @param els - A single element of type `P` or an array of elements of type `P` to be updated in the layer.
   */
  update_elements(els: P | P[]): void {
    if (!Array.isArray(els))
      els = [els];

    for (const el of els) {
      const circle = this.elements.get(el)!;
      circle.setLatLng(this.latlng_factory(el));
      if (this.radius_factory)
        circle.setRadius(this.radius_factory(el));
      if (this.color_factory)
        circle.setStyle({ color: this.color_factory(el) });
      if (this.popup_factory)
        circle.setPopupContent(this.popup_factory(el));
    }
  }

  /**
   * Removes elements from the layer.
   *
   * @param els - A single element of type `P` or an array of elements of type `P` to be removed from the layer.
   */
  remove_elements(els: P | P[]): void {
    if (!Array.isArray(els))
      els = [els];

    for (const el of els) {
      this.layer.removeLayer(this.elements.get(el)!);
      this.elements.delete(el);
    }
  }

  /**
   * Sets the factory function for creating the radius of the circle markers in the layer.
   *
   * @param factory - A function that takes an element of type `P` and returns a number for the radius.
   */
  protected set_radius_factory(factory: (element: P) => number): void {
    this.radius_factory = factory;
    for (const [el, circle] of this.elements)
      circle.setRadius(factory(el));
  }

  /**
   * Sets the factory function for creating the color of the circle markers in the layer.
   *
   * @param factory - A function that takes an element of type `P` and returns a string for the color.
   */
  protected set_color_factory(factory: (element: P) => string): void {
    this.color_factory = factory;
    for (const [el, circle] of this.elements)
      circle.setStyle({ color: factory(el) });
  }

  /**
   * Sets the factory function for creating popup content for the circle markers in the layer.
   *
   * @param factory - A function that takes an element of type `P` and returns a string for the popup content.
   */
  protected set_popup_factory(factory: (element: P) => string): void {
    this.popup_factory = factory;
    for (const [el, circle] of this.elements)
      circle.setPopupContent(factory(el));
  }
}

export class IconLayer<P> extends MapLayer<P> {

  private elements = new Map<P, L.Marker<P>>();
  private latlng_factory: (element: P) => L.LatLngExpression;
  private icon_factory?: (element: P) => L.Icon;
  private popup_factory?: (element: P) => string;

  constructor(latlng_factory: (element: P) => L.LatLngExpression, elements: P[] = []) {
    super();
    this.latlng_factory = latlng_factory;
    this.add_elements(elements);
  }

  /**
   * Adds elements to the layer, creating markers for each element with a specified icon.
   *
   * @param els - A single element of type `P` or an array of elements of type `P` to be added to the layer.
   */
  add_elements(els: P | P[]): void {
    if (!Array.isArray(els))
      els = [els];

    for (const el of els) {
      const marker = this.icon_factory ? L.marker<P>(this.latlng_factory(el), { icon: this.icon_factory(el) }) : L.marker<P>(this.latlng_factory(el));

      if (this.popup_factory)
        marker.bindPopup(this.popup_factory(el));

      this.layer.addLayer(marker);
      this.elements.set(el, marker);
    }
  }

  /**
   * Checks if an element is already present in the layer.
   *
   * @param el - The element of type `P` to check for existence in the layer.
   * @returns A boolean indicating whether the element exists in the layer.
   */
  has_element(el: P): boolean { return this.elements.has(el); }

  /**
   * Updates the position, icon, and popup content of existing elements in the layer.
   *
   * @param els - A single element of type `P` or an array of elements of type `P` to be updated in the layer.
   */
  update_elements(els: P | P[]): void {
    if (!Array.isArray(els))
      els = [els];

    for (const el of els) {
      const marker = this.elements.get(el)!;
      marker.setLatLng(this.latlng_factory(el));
      if (this.icon_factory)
        marker.setIcon(this.icon_factory(el));
      if (this.popup_factory)
        marker.setPopupContent(this.popup_factory(el));
    }
  }

  /**
   * Removes elements from the layer.
   *
   * @param els - A single element of type `P` or an array of elements of type `P` to be removed from the layer.
   */
  remove_elements(els: P | P[]): void {
    if (!Array.isArray(els))
      els = [els];

    for (const el of els) {
      this.layer.removeLayer(this.elements.get(el)!);
      this.elements.delete(el);
    }
  }

  /**
   * Sets the factory function for creating icons for the markers in the layer.
   *
   * @param factory - A function that takes an element of type `P` and returns a Leaflet `Icon` instance.
   */
  protected set_icon_factory(factory: (element: P) => L.Icon): void {
    this.icon_factory = factory;
    for (const [el, marker] of this.elements)
      marker.setIcon(factory(el));
  }

  /**
   * Sets the factory function for creating popup content for the markers in the layer.
   *
   * @param factory - A function that takes an element of type `P` and returns a string for the popup content.
   */
  protected set_popup_factory(factory: (element: P) => string): void {
    this.popup_factory = factory;
    for (const [el, marker] of this.elements)
      marker.setPopupContent(factory(el));
  }
}

export class ItemCircleLayer extends CircleLayer<coco.taxonomy.Item> implements coco.CoCoListener, coco.taxonomy.ItemListener {

  private type: coco.taxonomy.Type;

  constructor(type: coco.taxonomy.Type) {
    super(latlng_factory, Array.from(type.get_instances()).filter(is_static_located));
    this.type = type;

    const data = type.get_data();
    if (data && 'radius' in data)
      this.set_radius_factory(() => data.radius);
    if (data && 'color' in data)
      this.set_color_factory(() => data.color);
    const static_props = type.get_all_static_properties();
    if (static_props && 'radius' in static_props)
      this.set_radius_factory((item: coco.taxonomy.Item) => item.get_properties()!.radius as number);
    if (static_props && 'color' in static_props)
      this.set_color_factory((item: coco.taxonomy.Item) => item.get_properties()!.color as string);
    const dynamic_props = type.get_all_dynamic_properties();
    if (dynamic_props && 'radius' in dynamic_props)
      this.set_radius_factory((item: coco.taxonomy.Item) => item.get_datum()!.data.radius as number);
    if (dynamic_props && 'color' in dynamic_props)
      this.set_color_factory((item: coco.taxonomy.Item) => item.get_datum()!.data.color as string);

    this.set_popup_factory(popup_factory);
    for (const item of this.type.get_instances())
      if (is_static_located(item) || is_dynamic_located(item)) {
        this.add_elements(item);
        item.add_item_listener(this);
      }
    coco.CoCo.get_instance().add_coco_listener(this);
  }

  new_type(_type: coco.taxonomy.Type): void { }
  new_item(item: coco.taxonomy.Item): void {
    if (item.get_type().get_all_parents().has(this.type) && (is_static_located(item) || is_dynamic_located(item))) {
      this.add_elements(item);
      item.add_item_listener(this);
    }
  }
  new_intent(_intent: coco.llm.Intent): void { }
  new_entity(_entity: coco.llm.Entity): void { }

  properties_updated(item: coco.taxonomy.Item): void {
    if (is_static_located(item)) {
      if (!this.has_element(item)) {
        this.add_elements(item);
        item.add_item_listener(this);
      }
      else
        this.update_elements(item);
    }
    else
      this.remove_elements(item);
  }
  values_updated(_item: coco.taxonomy.Item): void { }
  new_value(item: coco.taxonomy.Item, _v: coco.taxonomy.Datum): void {
    if (is_dynamic_located(item)) {
      if (!this.has_element(item)) {
        this.add_elements(item);
        item.add_item_listener(this);
      }
      else
        this.update_elements(item);
    }
    else
      this.remove_elements(item);
  }
  slots_updated(_item: coco.taxonomy.Item): void { }

  override unmount(): void {
    super.unmount();
    coco.CoCo.get_instance().remove_coco_listener(this);
    for (const item of this.type.get_instances())
      item.remove_item_listener(this);
  }
}

export class ItemIconLayer extends IconLayer<coco.taxonomy.Item> implements coco.CoCoListener, coco.taxonomy.ItemListener {

  private type: coco.taxonomy.Type;

  constructor(type: coco.taxonomy.Type) {
    super(latlng_factory, Array.from(type.get_instances()).filter(is_static_located));
    this.type = type;

    const data = type.get_data();
    if (data && 'iconUrl' in data) {
      const icon = L.icon({ iconUrl: data.iconUrl });
      if ('iconSize' in data)
        icon.options.iconSize = [data.iconSize[0], data.iconSize[1]];
      this.set_icon_factory(() => icon);
    }
    const static_props = type.get_all_static_properties();
    if (static_props && 'iconUrl' in static_props)
      this.set_icon_factory((item: coco.taxonomy.Item) => {
        const icon = L.icon({ iconUrl: item.get_properties()!.iconUrl as string });
        if ('iconWidth' in item.get_properties()! && 'iconHeight' in item.get_properties()!)
          icon.options.iconSize = [item.get_properties()!.iconWidth as number, item.get_properties()!.iconHeight as number];
        return icon;
      });
    const dynamic_props = type.get_all_dynamic_properties();
    if (dynamic_props && 'iconUrl' in dynamic_props)
      this.set_icon_factory((item: coco.taxonomy.Item) => {
        const icon = L.icon({ iconUrl: item.get_datum()!.data.iconUrl as string });
        if ('iconWidth' in item.get_datum()!.data && 'iconHeight' in item.get_datum()!.data)
          icon.options.iconSize = [item.get_datum()!.data.iconWidth as number, item.get_datum()!.data.iconHeight as number];
        return icon;
      });

    this.set_popup_factory(popup_factory);
    for (const item of this.type.get_instances())
      if (is_static_located(item) || is_dynamic_located(item)) {
        this.add_elements(item);
        item.add_item_listener(this);
      }
    coco.CoCo.get_instance().add_coco_listener(this);
  }

  new_type(_type: coco.taxonomy.Type): void { }
  new_item(item: coco.taxonomy.Item): void {
    if (item.get_type().get_all_parents().has(this.type) && (is_static_located(item) || is_dynamic_located(item))) {
      this.add_elements(item);
      item.add_item_listener(this);
    }
  }
  new_intent(_intent: coco.llm.Intent): void { }
  new_entity(_entity: coco.llm.Entity): void { }

  properties_updated(item: coco.taxonomy.Item): void {
    if (is_static_located(item)) {
      if (!this.has_element(item)) {
        this.add_elements(item);
        item.add_item_listener(this);
      }
      else
        this.update_elements(item);
    }
    else
      this.remove_elements(item);
  }
  values_updated(_item: coco.taxonomy.Item): void { }
  new_value(item: coco.taxonomy.Item, _v: coco.taxonomy.Datum): void {
    if (is_dynamic_located(item)) {
      if (!this.has_element(item)) {
        this.add_elements(item);
        item.add_item_listener(this);
      }
      else
        this.update_elements(item);
    }
    else
      this.remove_elements(item);
  }
  slots_updated(_item: coco.taxonomy.Item): void { }

  override unmount(): void {
    super.unmount();
    coco.CoCo.get_instance().remove_coco_listener(this);
    for (const item of this.type.get_instances())
      item.remove_item_listener(this);
  }

  static is_icon_layer(type: coco.taxonomy.Type): boolean {
    const data = type.get_data();
    if (data && 'iconUrl' in data)
      return true;
    const static_props = type.get_all_static_properties();
    if (static_props && 'iconUrl' in static_props)
      return true;
    const dynamic_props = type.get_all_dynamic_properties();
    if (dynamic_props && 'iconUrl' in dynamic_props)
      return true;
    return false;
  }
}

export function is_type_located(type: coco.taxonomy.Type): boolean {
  const data = type.get_data();
  if (data && 'location' in data)
    return true;
  const static_props = type.get_all_static_properties();
  if (static_props && 'location' in static_props)
    return true;
  const dynamic_props = type.get_all_dynamic_properties();
  if (dynamic_props && 'lat' in dynamic_props && 'lng' in dynamic_props)
    return true;
  return false;
}

function is_static_located(item: coco.taxonomy.Item): boolean {
  const props = item.get_properties();
  return !!(props && 'location' in props);
}

function is_dynamic_located(item: coco.taxonomy.Item): boolean {
  const data = item.get_datum()?.data;
  return !!(data && 'lat' in data && 'lng' in data);
}

function latlng_factory(item: coco.taxonomy.Item): L.LatLngExpression {
  if (is_static_located(item)) {
    const loc = coco.CoCo.get_instance().get_item(item.get_properties()!.location as string);
    return [loc.get_properties()!.lat as number, loc.get_properties()!.lng as number];
  }
  else if (is_dynamic_located(item))
    return [item.get_datum()!.data.lat as number, item.get_datum()!.data.lng as number];
  else
    throw new Error(`Item ${item.to_string()} is not static or dynamic located.`);
}

function popup_factory(item: coco.taxonomy.Item): string {
  let popup = '';
  const props = item.get_properties();
  const data = item.get_datum()?.data;
  if (props)
    for (const key of Object.keys(props))
      if (key !== 'location')
        popup += `<b>${key}</b>: ${props[key]}<br>`;
  if (data)
    for (const key of Object.keys(data))
      if (key !== 'lat' && key !== 'lng')
        popup += `<b>${key}</b>: ${data[key]}<br>`;
  if (popup === '')
    return item.to_string();
  return popup;
}

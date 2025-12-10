import { App, Connection, Settings } from "@ratiosolver/flick";

export namespace coco {

  export class CoCo implements CoCoListener {

    private static instance: CoCo;
    private readonly property_types = new Map<string, taxonomy.PropertyType<any>>();
    private readonly types: Map<string, taxonomy.Type> = new Map();
    private readonly items: Map<string, taxonomy.Item> = new Map();
    private readonly coco_listeners: Set<CoCoListener> = new Set();

    private constructor() {
      this.add_property_type(new taxonomy.BoolPropertyType(this));
      this.add_property_type(new taxonomy.IntPropertyType(this));
      this.add_property_type(new taxonomy.FloatPropertyType(this));
      this.add_property_type(new taxonomy.StringPropertyType(this));
      this.add_property_type(new taxonomy.SymbolPropertyType(this));
      this.add_property_type(new taxonomy.ItemPropertyType(this));
      this.add_property_type(new taxonomy.JSONPropertyType(this));
    }

    static get_instance() {
      if (!CoCo.instance)
        CoCo.instance = new CoCo();
      return CoCo.instance;
    }

    add_property_type(pt: taxonomy.PropertyType<any>) { this.property_types.set(pt.get_name(), pt); }

    get_types(): MapIterator<taxonomy.Type> { return this.types.values(); }
    get_items(): MapIterator<taxonomy.Item> { return this.items.values(); }

    new_type(type: taxonomy.Type): void {
      this.types.set(type.get_name(), type);
      for (const listener of this.coco_listeners) listener.new_type(type);
    }

    new_item(item: taxonomy.Item): void {
      this.items.set(item.get_id(), item);
      for (const tp of item.get_types())
        tp._instances.add(item);
      for (const listener of this.coco_listeners) listener.new_item(item);
    }

    /**
     * Loads data for a given taxonomy item within a specified date range.
     *
     * @param item - The taxonomy item for which data is to be loaded.
     * @param from - The start date of the range (in milliseconds since the Unix epoch). Defaults to 14 days ago.
     * @param to - The end date of the range (in milliseconds since the Unix epoch). Defaults to the current date and time.
     *
     * If the request is successful, the item's values are updated with the fetched data.
     * If the request fails, an error message is displayed using the application's toast mechanism.
     */
    load_data(item: taxonomy.Item, from = Date.now() - 1000 * 60 * 60 * 24 * 14, to = Date.now()): void {
      console.debug('Loading data for ', item.to_string(), ' from ', new Date(from), ' to ', new Date(to));
      const headers: { 'content-type': string, 'authorization'?: string } = { 'content-type': 'application/json' };
      const token = Connection.get_instance().get_token();
      if (token)
        headers['authorization'] = `Bearer ${token}`;
      fetch(Settings.get_instance().get_host() + '/data/' + item.get_id() + '?' + new URLSearchParams({ from: from.toString(), to: to.toString() }), { method: 'GET', headers }).then(res => {
        if (res.ok)
          res.json().then(data => item._set_data(data));
        else
          res.json().then(data => App.get_instance().toast(data.message)).catch(err => console.error(err));
      });
    }

    /**
     * Publishes data associated with a given taxonomy item to the server.
     *
     * Sends a POST request to the server endpoint `/data/{item_id}` with the provided data
     * serialized as JSON. Includes an authorization header if a token is available.
     * Displays a toast notification with an error message if the request fails.
     *
     * @param item - The taxonomy item for which the data is being published.
     * @param data - The data to be published, represented as a record of key-value pairs.
     */
    publish(item: taxonomy.Item, data: Record<string, unknown>): void {
      console.debug('Publishing data for ', item.to_string(), ': ', JSON.stringify(data));
      const headers: { 'content-type': string, 'authorization'?: string } = { 'content-type': 'application/json' };
      const token = Connection.get_instance().get_token();
      if (token)
        headers['authorization'] = `Bearer ${token}`;
      fetch(Settings.get_instance().get_host() + '/data/' + item.get_id(), { method: 'POST', headers, body: JSON.stringify(data) }).then(res => {
        if (!res.ok)
          res.json().then(data => App.get_instance().toast(data.message)).catch(err => console.error(err));
      });
    }

    /**
     * Generates and retrieves fake data for a given taxonomy type from the backend.
     *
     * @param type - The taxonomy type for which to generate fake data.
     * @param pars - Optional array of string parameters to be sent as query parameters.
     * @returns A promise that resolves to a record containing the fake data.
     *
     * @remarks
     * - Adds an authorization header if a token is available.
     * - Displays a toast notification with an error message if the request fails.
     * - Logs debug information about the type and parameters.
     */
    async fake_data(type: taxonomy.Type, pars: string[] | undefined = undefined): Promise<Record<string, unknown>> {
      console.debug('Faking data for ', type.get_name());
      const headers: { 'content-type': string, 'authorization'?: string } = { 'content-type': 'application/json' };
      const token = Connection.get_instance().get_token();
      if (token)
        headers['authorization'] = `Bearer ${token}`;
      let url = Settings.get_instance().get_host() + '/fake/' + type.get_name();
      if (pars) {
        console.debug('Parameters:', JSON.stringify(pars));
        url += '?' + new URLSearchParams({ parameters: JSON.stringify(pars) });
      }

      const res = await fetch(url, { method: 'GET', headers });
      if (res.ok)
        return res.json();
      else {
        res.json().then(data => App.get_instance().toast(data.message)).catch(err => console.error(err));
        return {};
      }
    }

    update_coco(message: UpdateCoCoMessage | any): void {
      if ('msg_type' in message)
        switch (message.msg_type) {
          case 'coco':
            this.init(message as CoCoMessage);
            break;
          case 'new_type':
            const ntm = message as NewTypeMessage;
            const n_tp = new taxonomy.Type(ntm.name, ntm.data);
            this.new_type(n_tp);
            this.refine_type(n_tp, ntm);
            break;
          case 'new_item':
            const nim = message as NewItemMessage;
            this.new_item(this.make_item(nim.id, nim));
            break;
          case 'updated_item':
            const uim = message as UpdatedItemMessage;
            const item = this.get_item(uim.id);
            if (uim.types)
              item._set_types(new Set(uim.types.map(tn => this.get_type(tn)!)));
            if (uim.properties)
              item._set_properties(uim.properties);
            break;
          case 'new_data':
            const ndm = message as NewItemMessage;
            this.get_item(ndm.id)._set_datum({ data: ndm.value!.data, timestamp: new Date(ndm.value!.timestamp) });
            break;
        }
    }

    private init(coco_message: CoCoMessage): void {
      if (coco_message.types) {
        this.types.clear();
        for (const [name, tp] of Object.entries(coco_message.types))
          this.new_type(new taxonomy.Type(name, tp.data));
        for (const [name, tp] of Object.entries(coco_message.types)) {
          const ctp = this.types.get(name)!;
          this.refine_type(ctp, tp);
        }
      }
      if (coco_message.items) {
        this.items.clear();
        for (const [id, itm] of Object.entries(coco_message.items))
          this.new_item(this.make_item(id, itm));
      }
    }

    private refine_type(tp: taxonomy.Type, tpm: TypeMessage) {
      if (tpm.static_properties) {
        const static_props = new Map<string, taxonomy.Property<unknown>>();
        for (const [name, prop] of Object.entries(tpm.static_properties))
          static_props.set(name, this.property_types.get(prop.type)!.make_property(prop));
        tp._set_static_properties(static_props);
      }
      if (tpm.dynamic_properties) {
        const dynamic_props = new Map<string, taxonomy.Property<unknown>>();
        for (const [name, prop] of Object.entries(tpm.dynamic_properties))
          dynamic_props.set(name, this.property_types.get(prop.type)!.make_property(prop));
        tp._set_dynamic_properties(dynamic_props);
      }
    }

    private make_item(id: string, itm: ItemMessage): taxonomy.Item {
      let value = undefined;
      if (itm.value)
        value = { data: itm.value.data, timestamp: new Date(itm.value.timestamp) };
      const types: Set<taxonomy.Type> = new Set();
      if (itm.types)
        for (const tn of itm.types)
          types.add(this.get_type(tn)!);
      return new taxonomy.Item(id, types, itm.properties, value, (itm as any).slots);
    }

    get_type(name: string): taxonomy.Type { return this.types.get(name)!; }
    get_item(id: string): taxonomy.Item { return this.items.get(id)!; }

    add_coco_listener(listener: CoCoListener) { this.coco_listeners.add(listener); }
    remove_coco_listener(listener: CoCoListener) { this.coco_listeners.delete(listener); }
  }

  export interface CoCoListener {

    new_type(type: taxonomy.Type): void;
    new_item(item: taxonomy.Item): void;
  }

  export namespace taxonomy {

    export abstract class PropertyType<P extends Property<unknown>> {

      protected readonly cc: CoCo;
      private readonly name: string;

      constructor(cc: CoCo, name: string) {
        this.cc = cc;
        this.name = name;
      }

      get_name(): string { return this.name; }

      abstract make_property(property_message: PropertyMessage | any): P;

      abstract to_string(val: unknown): string;
    }

    export class BoolPropertyType extends PropertyType<BoolProperty> {

      constructor(cc: CoCo) { super(cc, 'bool'); }

      override make_property(property_message: PropertyMessage | any): BoolProperty {
        const b_pm = property_message as BoolPropertyMessage;
        return new BoolProperty(this, b_pm.multiple ?? false, b_pm.default);
      }

      to_string(val: boolean): string { return val ? 'true' : 'false'; }
    }

    export class IntPropertyType extends PropertyType<IntProperty> {

      constructor(cc: CoCo) { super(cc, 'int'); }

      override make_property(property_message: PropertyMessage | any): IntProperty {
        const i_pm = property_message as IntPropertyMessage;
        return new IntProperty(this, i_pm.multiple ?? false, i_pm.min ?? Number.NEGATIVE_INFINITY, i_pm.max ?? Number.POSITIVE_INFINITY, i_pm.default);
      }

      to_string(val: number): string { return String(val); }
    }

    export class FloatPropertyType extends PropertyType<FloatProperty> {

      constructor(cc: CoCo) { super(cc, 'float'); }

      override make_property(property_message: PropertyMessage | any): FloatProperty {
        const f_pm = property_message as FloatPropertyMessage;
        return new FloatProperty(this, f_pm.multiple ?? false, f_pm.min ?? Number.NEGATIVE_INFINITY, f_pm.max ?? Number.POSITIVE_INFINITY, f_pm.default);
      }

      to_string(val: number): string { return String(val); }
    }

    export class StringPropertyType extends PropertyType<StringProperty> {

      constructor(cc: CoCo) { super(cc, 'string'); }

      override make_property(property_message: PropertyMessage | any): StringProperty {
        const s_pm = property_message as StringPropertyMessage;
        return new StringProperty(this, s_pm.multiple ?? false, s_pm.default);
      }

      to_string(val: string | null): string { return val ? val : ''; }
    }

    export class SymbolPropertyType extends PropertyType<SymbolProperty> {

      constructor(cc: CoCo) { super(cc, 'symbol'); }

      override make_property(property_message: PropertyMessage | any): SymbolProperty {
        const sym_pm = property_message as SymbolPropertyMessage;
        return new SymbolProperty(this, sym_pm.multiple ?? false, sym_pm.values, sym_pm.default);
      }

      to_string(val: string | string[] | null): string {
        if (val === null) return '';
        if (Array.isArray(val)) return '[' + val.join(', ') + ']';
        return val;
      }
    }

    export class ItemPropertyType extends PropertyType<ItemProperty> {

      constructor(cc: CoCo) { super(cc, 'item'); }

      override make_property(property_message: PropertyMessage | any): ItemProperty {
        const itm_pm = property_message as ItemPropertyMessage;
        let def = undefined;
        if (itm_pm.default)
          def = Array.isArray(itm_pm.default) ? itm_pm.default.map(itm => this.cc.get_item(itm)) : this.cc.get_item(itm_pm.default);
        return new ItemProperty(this, this.cc.get_type(itm_pm.domain), itm_pm.multiple ?? false, def);
      }

      to_string(val: string | string[] | null): string {
        if (val === null) return '';
        if (Array.isArray(val)) return '[' + val.map(id => this.cc.get_item(id).to_string()).join(', ') + ']';
        return this.cc.get_item(val).to_string();
      }
    }

    export class JSONPropertyType extends PropertyType<JSONProperty> {

      constructor(cc: CoCo) { super(cc, 'json'); }

      override make_property(property_message: PropertyMessage | any): JSONProperty {
        const j_pm = property_message as JSONPropertyMessage;
        return new JSONProperty(this, j_pm.schema, j_pm.default);
      }

      to_string(val: JSON): string { return JSON.stringify(val); }
    }

    export class Property<V> {

      protected readonly type: PropertyType<Property<V>>;
      protected readonly default_value: V | undefined;

      constructor(type: PropertyType<Property<V>>, default_value?: V) {
        this.type = type;
        this.default_value = default_value;
      }

      get_type(): PropertyType<Property<V>> { return this.type; }

      has_default_value(): boolean { return this.default_value !== undefined; }

      get_default_value(): V | undefined { return this.default_value; }

      to_string(): string { return this.default_value ? `(${this.default_value})` : ''; }
    }

    export class BoolProperty extends Property<boolean> {

      private readonly multiple: boolean;

      constructor(type: BoolPropertyType, multiple: boolean, default_value?: boolean) {
        super(type, default_value);
        this.multiple = multiple;
      }

      is_multiple(): boolean { return this.multiple; }

      override to_string(): string {
        const str: string = 'bool';
        if (this.has_default_value())
          str.concat(' (' + (this.default_value ? 'true' : 'false') + ')');
        return str;
      }
    }

    export class IntProperty extends Property<number> {

      private readonly multiple: boolean;
      private readonly min: number;
      private readonly max: number;

      constructor(type: IntPropertyType, multiple: boolean, min: number, max: number, default_value?: number) {
        super(type, default_value);
        this.multiple = multiple;
        this.min = min;
        this.max = max;
      }

      is_multiple(): boolean { return this.multiple; }
      get_min(): number { return this.min; }
      get_max(): number { return this.max; }

      override to_string(): string {
        const str: string = 'int';
        if (this.min && this.max)
          str.concat(` [${this.min}, ${this.max}]`);
        if (this.has_default_value())
          str.concat(' ' + String(this.default_value));
        return str;
      }
    }

    export class FloatProperty extends Property<number> {

      private readonly multiple: boolean;
      private readonly min: number;
      private readonly max: number;

      constructor(type: FloatPropertyType, multiple: boolean, min: number, max: number, default_value?: number) {
        super(type, default_value);
        this.multiple = multiple;
        this.min = min;
        this.max = max;
      }

      is_multiple(): boolean { return this.multiple; }
      get_min(): number { return this.min; }
      get_max(): number { return this.max; }

      override to_string(): string {
        const str: string = 'float';
        if (this.min && this.max)
          str.concat(` [${this.min}, ${this.max}]`);
        if (this.has_default_value())
          str.concat(' ' + String(this.default_value));
        return str;
      }
    }

    export class StringProperty extends Property<string | null> {

      private readonly multiple: boolean;

      constructor(type: StringPropertyType, multiple: boolean, default_value?: string) {
        super(type, default_value);
        this.multiple = multiple;
      }

      is_multiple(): boolean { return this.multiple; }

      override to_string(): string {
        const str: string = 'string';
        if (this.has_default_value())
          str.concat(' (' + this.default_value + ')');
        return str;
      }
    }

    export class SymbolProperty extends Property<string | string[] | null> {

      private readonly multiple: boolean;
      private readonly symbols?: string[];

      constructor(type: SymbolPropertyType, multiple: boolean, symbols?: string[], default_value?: string | string[]) {
        super(type, default_value);
        this.multiple = multiple;
        this.symbols = symbols;
      }

      is_multiple(): boolean { return this.multiple; }
      get_symbols(): string[] | undefined { return this.symbols; }

      override to_string(): string {
        const str: string = 'symbol';
        if (this.multiple)
          str.concat(' [multiple]');
        if (this.symbols)
          str.concat(' {' + this.symbols.join(', ') + '}');
        if (this.default_value)
          str.concat(' (' + (Array.isArray(this.default_value) ? this.default_value.join(', ') : this.default_value) + ')');
        return str;
      }
    }

    export class ItemProperty extends Property<Item | Item[] | null> {

      private readonly domain: Type;
      private readonly multiple: boolean;

      constructor(type: ItemPropertyType, domain: Type, multiple: boolean, default_value?: Item | Item[]) {
        super(type, default_value);
        this.domain = domain;
        this.multiple = multiple;
      }

      get_domain(): Type { return this.domain; }
      is_multiple(): boolean { return this.multiple; }

      override to_string(): string {
        const str: string = this.domain.get_name();
        if (this.multiple)
          str.concat(' [multiple]');
        if (this.default_value)
          str.concat(' (' + (Array.isArray(this.default_value) ? this.default_value.map(itm => itm.to_string()).join(', ') : this.default_value.to_string()) + ')');
        return str;
      }
    }

    export class JSONProperty extends Property<Record<string, any> | null> {

      private readonly schema: Record<string, any>;

      constructor(type: JSONPropertyType, schema: Record<string, any>, default_value?: Record<string, any>) {
        super(type, default_value);
        this.schema = schema;
      }

      override to_string(): string { return JSON.stringify(this.schema); }
    }

    export class Type {

      private readonly name: string;
      private data?: Record<string, any>;
      private static_properties?: Map<string, Property<unknown>>;
      private dynamic_properties?: Map<string, Property<unknown>>;
      readonly _instances: Set<Item> = new Set();
      private readonly listeners = new Set<TypeListener>();

      constructor(name: string, data?: Record<string, any>, static_properties?: Map<string, Property<unknown>>, dynamic_properties?: Map<string, Property<unknown>>) {
        this.name = name;
        this.data = data;
        this.static_properties = static_properties;
        this.dynamic_properties = dynamic_properties;
      }

      get_name(): string { return this.name; }
      get_data(): Record<string, any> | undefined { return this.data; }
      get_static_properties(): Map<string, Property<unknown>> | undefined { return this.static_properties; }
      get_dynamic_properties(): Map<string, Property<unknown>> | undefined { return this.dynamic_properties; }
      get_instances(): Set<Item> { return this._instances; }

      _set_data(data?: Record<string, any>): void {
        this.data = data;
        for (const l of this.listeners) l.data_updated(this);
      }
      _set_static_properties(sp?: Map<string, Property<unknown>>): void {
        this.static_properties = sp;
        for (const l of this.listeners) l.static_properties_updated(this);
      }
      _set_dynamic_properties(dp?: Map<string, Property<unknown>>): void {
        this.dynamic_properties = dp;
        for (const l of this.listeners) l.dynamic_properties_updated(this);
      }

      to_string(): string {
        let tp_str = `<html><b>${this.name}</b>`;
        if (this.static_properties) {
          tp_str += '<br><b>Static Properties:</b>';
          for (const [name, prop] of this.static_properties)
            tp_str += `<br><em>${name}</em> ${prop.to_string()}`;
        }
        if (this.dynamic_properties) {
          tp_str += '<br><b>Dynamic Properties:</b>';
          for (const [name, prop] of this.dynamic_properties)
            tp_str += `<br><em>${name}</em> ${prop.to_string()}`;
        }
        return tp_str + '</html>';
      }

      add_type_listener(l: TypeListener): void { this.listeners.add(l); }
      remove_type_listener(l: TypeListener): void { this.listeners.delete(l); }
    }

    export interface TypeListener {

      data_updated(type: Type): void;
      static_properties_updated(type: Type): void;
      dynamic_properties_updated(type: Type): void;
    }

    export type Datum = { data: Record<string, unknown>, timestamp: Date }

    export class Item {

      private readonly id: string;
      private readonly types: Set<Type> = new Set();
      private properties?: Record<string, unknown>;
      private datum?: Datum;
      private slots?: Record<string, unknown>;
      private data: Datum[] = [];
      private readonly listeners = new Set<ItemListener>();

      constructor(id: string, types: Set<Type>, properties?: Record<string, unknown>, value?: Datum, slots?: Record<string, unknown>) {
        this.id = id;
        for (const tp of types) {
          this.types.add(tp);
          tp._instances.add(this);
        }
        this.properties = properties;
        this.datum = value;
        this.slots = slots;
      }

      get_id(): string { return this.id; }
      get_types(): Set<Type> { return this.types; }
      get_properties(): Record<string, unknown> | undefined { return this.properties; }
      get_datum(): Datum | undefined { return this.datum; }
      get_data(): Datum[] { return this.data; }
      get_slots(): Record<string, unknown> | undefined { return this.slots; }

      _set_types(types: Set<Type>): void {
        for (const tp of this.types)
          tp._instances.delete(this);
        this.types.clear();
        for (const tp of types) {
          this.types.add(tp);
          tp._instances.add(this);
        }
        for (const l of this.listeners) l.types_updated(this);
      }
      _set_properties(props: Record<string, unknown>): void {
        this.properties = props;
        for (const l of this.listeners) l.properties_updated(this);
      }
      _set_data(data: Datum[]): void {
        this.data = data;
        for (const l of this.listeners) l.values_updated(this);
      }
      _set_datum(datum: Datum): void {
        this.datum = datum;
        this.data.push(datum);
        for (const l of this.listeners) l.new_value(this, datum);
      }
      _set_slots(slots?: Record<string, unknown>): void {
        this.slots = slots;
        for (const l of this.listeners) l.slots_updated(this);
      }

      add_item_listener(l: ItemListener): void { this.listeners.add(l); }
      remove_item_listener(l: ItemListener): void { this.listeners.delete(l); }

      to_string(): string { return this.properties && 'name' in this.properties ? this.properties.name as string : this.id; }
    }

    export interface ItemListener {

      types_updated(item: Item): void;
      properties_updated(item: Item): void;
      values_updated(item: Item): void;
      new_value(item: Item, v: Datum): void;
      slots_updated(item: Item): void;
    }

    export function get_static_properties(item: Item): Map<string, Property<unknown>> {
      const static_props = new Map<string, Property<unknown>>();
      for (const tp of item.get_types()) {
        const tp_static_props = tp.get_static_properties();
        if (tp_static_props)
          for (const [name, prop] of tp_static_props)
            static_props.set(name, prop);
      }
      return static_props;
    }

    export function get_dynamic_properties(item: Item): Map<string, Property<unknown>> {
      const dynamic_props = new Map<string, Property<unknown>>();
      for (const tp of item.get_types()) {
        const tp_dynamic_props = tp.get_dynamic_properties();
        if (tp_dynamic_props)
          for (const [name, prop] of tp_dynamic_props)
            dynamic_props.set(name, prop);
      }
      return dynamic_props;
    }
  }

  export class User {

    private readonly id: string;
    private readonly personal_data: Record<string, unknown>;

    constructor(id: string, personal_data: Record<string, unknown>) {
      this.id = id;
      this.personal_data = personal_data;
    }

    get_id(): string { return this.id; }
    get_personal_data(): Record<string, unknown> { return this.personal_data; }
  }

  export namespace llm {

    export class Intent {

      private readonly name: string;
      private readonly description: string;

      constructor(name: string, description: string) {
        this.name = name;
        this.description = description;
      }

      get_name(): string { return this.name; }
      get_description(): string { return this.description; }
    }

    export enum EntityType {
      string,
      int,
      float,
      bool,
      symbol
    }

    export class Entity {

      private readonly name: string;
      private readonly type: EntityType;
      private readonly description: string;

      constructor(name: string, type: EntityType, description: string) {
        this.name = name;
        this.type = type;
        this.description = description;
      }

      get_name(): string { return this.name; }
      get_type(): EntityType { return this.type; }
      get_description(): string { return this.description; }
    }

    export class Slot {

      private readonly name: string;
      private readonly type: EntityType;
      private readonly description: string;
      private readonly influence_context: boolean;

      constructor(name: string, type: EntityType, description: string, influence_context: boolean) {
        this.name = name;
        this.type = type;
        this.description = description;
        this.influence_context = influence_context;
      }

      get_name(): string { return this.name; }
      get_type(): EntityType { return this.type; }
      get_description(): string { return this.description; }
      is_influencing_context(): boolean { return this.influence_context; }
    }
  }
}

type UpdateCoCoMessage = { msg_type: string } & (CoCoMessage | NewTypeMessage | NewItemMessage | NewDataMessage);

interface CoCoMessage {
  types?: Record<string, TypeMessage>;
  items?: Record<string, ItemMessage>;
}

interface NewTypeMessage extends TypeMessage {
  name: string;
}

interface NewItemMessage extends ItemMessage {
  id: string;
}

interface UpdatedItemMessage {
  id: string;
  types?: string[];
  properties?: Record<string, unknown>;
}

interface NewDataMessage extends ValueMessage {
  id: string;
}

interface PropMessage<V> {
  type: string;
  default?: V;
}

interface BoolPropertyMessage extends PropMessage<boolean> {
  multiple?: boolean;
}

interface IntPropertyMessage extends PropMessage<number> {
  multiple?: boolean;
  min?: number;
  max?: number;
}

interface FloatPropertyMessage extends PropMessage<number> {
  multiple?: boolean;
  min?: number;
  max?: number;
}

interface StringPropertyMessage extends PropMessage<string> {
  multiple?: boolean;
}

interface SymbolPropertyMessage extends PropMessage<string | string[]> {
  multiple?: boolean;
  values?: string[];
}

interface ItemPropertyMessage extends PropMessage<string | string[]> {
  domain: string;
  multiple?: boolean;
}

interface JSONPropertyMessage extends PropMessage<Record<string, any>> {
  schema: Record<string, any>;
}

type PropertyMessage = BoolPropertyMessage | IntPropertyMessage | FloatPropertyMessage | StringPropertyMessage | SymbolPropertyMessage | ItemPropertyMessage | JSONPropertyMessage;

interface TypeMessage {
  data?: Record<string, any>;
  static_properties?: Record<string, PropertyMessage>;
  dynamic_properties?: Record<string, PropertyMessage>;
}

interface ValueMessage {
  data: Record<string, unknown>;
  timestamp: number;
}

interface ItemMessage {
  types?: string[];
  properties?: Record<string, unknown>;
  value?: ValueMessage;
}
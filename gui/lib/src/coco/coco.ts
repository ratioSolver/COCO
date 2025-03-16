export namespace coco {

  export class CoCo implements CoCoListener {

    private static instance: CoCo;
    private readonly property_types = new Map<string, taxonomy.PropertyType<any>>();
    private readonly types: Map<string, taxonomy.Type>;
    private readonly items: Map<string, taxonomy.Item>;
    private readonly coco_listeners: Set<CoCoListener> = new Set();

    private constructor() {
      this.add_property_type(new taxonomy.BoolPropertyType(this));
      this.add_property_type(new taxonomy.IntPropertyType(this));
      this.add_property_type(new taxonomy.FloatPropertyType(this));
      this.add_property_type(new taxonomy.StringPropertyType(this));
      this.add_property_type(new taxonomy.SymbolPropertyType(this));
      this.add_property_type(new taxonomy.ItemPropertyType(this));
      this.add_property_type(new taxonomy.JSONPropertyType(this));
      this.types = new Map();
      this.items = new Map();
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
      for (const listener of this.coco_listeners) listener.new_item(item);
    }

    update_coco(message: UpdateCoCoMessage | any): void {
      switch (message.type) {
        case 'coco':
          this.init(message as CoCoMessage);
          break;
        case 'new_type':
          const ntm = message as NewTypeMessage;
          const n_tp = new taxonomy.Type(ntm.name, [], ntm.data);
          this.new_type(n_tp);
          this.refine_type(n_tp, ntm);
          break;
        case 'new_item':
          const nim = message as NewItemMessage;
          this.new_item(this.make_item(nim.id, nim));
          break;
        case 'new_data':
          const ndm = message as NewItemMessage;
          this.get_item(ndm.id)._add_value({ data: ndm.value!.data, timestamp: new Date(ndm.value!.timestamp) });
      }
    }

    private init(coco_message: CoCoMessage): void {
      if (coco_message.types) {
        this.types.clear();
        for (const [name, tp] of Object.entries(coco_message.types))
          this.new_type(new taxonomy.Type(name, [], tp.data));
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
      if (tpm.parents)
        tp._set_parents(tpm.parents.map(p => this.types.get(p)!));
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
      return new taxonomy.Item(id, this.types.get(itm.type)!, itm.properties, value);
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
        return new BoolProperty(this, b_pm.default_value);
      }

      to_string(val: boolean): string { return val ? 'true' : 'false'; }
    }

    export class IntPropertyType extends PropertyType<IntProperty> {

      constructor(cc: CoCo) { super(cc, 'int'); }

      override make_property(property_message: PropertyMessage | any): IntProperty {
        const i_pm = property_message as IntPropertyMessage;
        return new IntProperty(this, i_pm.min, i_pm.max, i_pm.default_value);
      }

      to_string(val: number): string { return String(val); }
    }

    export class FloatPropertyType extends PropertyType<FloatProperty> {

      constructor(cc: CoCo) { super(cc, 'float'); }

      override make_property(property_message: PropertyMessage | any): FloatProperty {
        const f_pm = property_message as FloatPropertyMessage;
        return new FloatProperty(this, f_pm.min, f_pm.max, f_pm.default_value);
      }

      to_string(val: number): string { return String(val); }
    }

    export class StringPropertyType extends PropertyType<StringProperty> {

      constructor(cc: CoCo) { super(cc, 'string'); }

      override make_property(property_message: PropertyMessage | any): StringProperty {
        const s_pm = property_message as StringPropertyMessage;
        return new StringProperty(this, s_pm.default_value);
      }

      to_string(val: string): string { return val; }
    }

    export class SymbolPropertyType extends PropertyType<SymbolProperty> {

      constructor(cc: CoCo) { super(cc, 'symbol'); }

      override make_property(property_message: PropertyMessage | any): SymbolProperty {
        const sym_pm = property_message as SymbolPropertyMessage;
        return new SymbolProperty(this, sym_pm.multiple, sym_pm.symbols, sym_pm.default_value);
      }

      to_string(val: string | string[]): string { return Array.isArray(val) ? val.join(', ') : val; }
    }

    export class ItemPropertyType extends PropertyType<ItemProperty> {

      constructor(cc: CoCo) { super(cc, 'item'); }

      override make_property(property_message: PropertyMessage | any): ItemProperty {
        const itm_pm = property_message as ItemPropertyMessage;
        let def = undefined;
        if (itm_pm.default_value)
          def = Array.isArray(itm_pm.default_value) ? itm_pm.default_value.map(itm => this.cc.get_item(itm)) : this.cc.get_item(itm_pm.default_value);
        return new ItemProperty(this, this.cc.get_type(itm_pm.domain), itm_pm.multiple, itm_pm.items?.map(itm => this.cc.get_item(itm)), def);
      }

      to_string(val: string | string[]): string { return Array.isArray(val) ? '[' + val.map(id => this.cc.get_item(id).to_string()).join(', ') + ']' : this.cc.get_item(val).to_string(); }
    }

    export class JSONPropertyType extends PropertyType<JSONProperty> {

      constructor(cc: CoCo) { super(cc, 'json'); }

      override make_property(property_message: PropertyMessage | any): JSONProperty {
        const j_pm = property_message as JSONPropertyMessage;
        return new JSONProperty(this, j_pm.schema, j_pm.default_value);
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

      to_string(): string { return this.default_value ? `(${this.default_value})` : ''; }
    }

    export class BoolProperty extends Property<boolean> {

      constructor(type: BoolPropertyType, default_value?: boolean) {
        super(type, default_value);
      }
    }

    export class IntProperty extends Property<number> {

      private readonly min?: number; max?: number;

      constructor(type: IntPropertyType, min?: number, max?: number, default_value?: number) {
        super(type, default_value);
        this.min = min;
        this.max = max;
      }

      override to_string(): string {
        const str: string = '';
        if (this.min && this.max)
          str.concat(`[${this.min}, ${this.max}] `);
        if (this.default_value)
          str.concat(String(this.default_value));
        return str;
      }
    }

    export class FloatProperty extends Property<number> {

      private readonly min?: number; max?: number;

      constructor(type: FloatPropertyType, min?: number, max?: number, default_value?: number) {
        super(type, default_value);
        this.min = min;
        this.max = max;
      }

      override to_string(): string {
        const str: string = '';
        if (this.min && this.max)
          str.concat(`[${this.min}, ${this.max}] `);
        if (this.default_value)
          str.concat(String(this.default_value));
        return str;
      }
    }

    export class StringProperty extends Property<string> {
      constructor(type: StringPropertyType, default_value?: string) {
        super(type, default_value);
      }
    }

    export class SymbolProperty extends Property<string | string[]> {

      private readonly multiple: boolean;
      private readonly symbols?: string[];

      constructor(type: SymbolPropertyType, multiple: boolean, symbols?: string[], default_value?: string | string[]) {
        super(type, default_value);
        this.multiple = multiple;
        this.symbols = symbols;
      }

      override to_string(): string {
        const str: string = this.multiple ? '[multiple]' : '';
        if (this.symbols) {
          str.concat(' ');
          str.concat('{' + this.symbols.join(', ') + '}');
        }
        if (this.default_value)
          str.concat('(' + (Array.isArray(this.default_value) ? this.default_value.join(', ') : this.default_value) + ')');
        return str;
      }
    }

    export class ItemProperty extends Property<Item | Item[]> {

      private readonly domain: Type;
      private readonly multiple: boolean;
      private readonly symbols?: Item[];

      constructor(type: ItemPropertyType, domain: Type, multiple: boolean, symbols?: Item[], default_value?: Item | Item[]) {
        super(type, default_value);
        this.domain = domain;
        this.multiple = multiple;
        this.symbols = symbols;
      }

      get_domain(): Type { return this.domain; }

      override to_string(): string {
        const str: string = this.multiple ? '[multiple]' : '';
        if (this.symbols) {
          str.concat(' ');
          str.concat('{' + this.symbols.map(itm => itm.to_string()).join(', ') + '}');
        }
        if (this.default_value)
          str.concat('(' + (Array.isArray(this.default_value) ? this.default_value.map(itm => itm.to_string()).join(', ') : this.default_value.to_string()) + ')');
        return str;
      }
    }

    export class JSONProperty extends Property<Record<string, any>> {

      private readonly schema: Record<string, any>;

      constructor(type: JSONPropertyType, schema: Record<string, any>, default_value?: Record<string, any>) {
        super(type, default_value);
        this.schema = schema;
      }

      override to_string(): string { return JSON.stringify(this.schema); }
    }

    export class Type {

      private readonly name: string;
      private parents?: Type[];
      private data?: Record<string, any>;
      private static_properties?: Map<string, Property<unknown>>;
      private dynamic_properties?: Map<string, Property<unknown>>;
      private readonly listeners = new Set<TypeListener>();

      constructor(name: string, parents?: Type[], data?: Record<string, any>, static_properties?: Map<string, Property<unknown>>, dynamic_properties?: Map<string, Property<unknown>>) {
        this.name = name;
        this.parents = parents;
        this.data = data;
        this.static_properties = static_properties;
        this.dynamic_properties = dynamic_properties;
      }

      get_name(): string { return this.name; }
      get_parents(): Type[] | undefined { return this.parents; }
      get_data(): Record<string, any> | undefined { return this.data; }
      get_static_properties(): Map<string, Property<unknown>> | undefined { return this.static_properties; }
      get_all_static_properties(): Map<string, Property<unknown>> {
        const props = new Map<string, coco.taxonomy.Property<unknown>>();
        const q: Type[] = [this];
        while (q.length > 0) {
          const t = q.shift()!;
          if (t.static_properties)
            for (const [name, property] of t.static_properties)
              props.set(name, property);
          if (t.parents)
            for (const par of t.parents)
              q.push(par);
        }
        return props;
      }
      get_dynamic_properties(): Map<string, Property<unknown>> | undefined { return this.dynamic_properties; }
      get_all_dynamic_properties(): Map<string, Property<unknown>> {
        const props = new Map<string, coco.taxonomy.Property<unknown>>();
        const q: Type[] = [this];
        while (q.length > 0) {
          const t = q.shift()!;
          if (t.dynamic_properties)
            for (const [name, property] of t.dynamic_properties)
              props.set(name, property);
          if (t.parents)
            for (const par of t.parents)
              q.push(par);
        }
        return props;
      }

      _set_parents(ps?: Type[]): void {
        this.parents = ps;
        for (const l of this.listeners) l.parents_updated(this);
      }
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

      parents_updated(type: Type): void;
      data_updated(type: Type): void;
      static_properties_updated(type: Type): void;
      dynamic_properties_updated(type: Type): void;
    }

    export type Value = { data: Record<string, unknown>, timestamp: Date }

    export class Item {

      private readonly id: string;
      private readonly type: Type;
      private properties?: Record<string, unknown>;
      private value?: Value;
      private values: Value[] = [];
      private readonly listeners = new Set<ItemListener>();

      constructor(id: string, type: Type, properties?: Record<string, unknown>, value?: Value) {
        this.id = id;
        this.type = type;
        this.properties = properties;
        this.value = value;
      }

      get_id(): string { return this.id; }
      get_type(): Type { return this.type; }
      get_properties(): Record<string, unknown> | undefined { return this.properties; }
      get_value(): Value | undefined { return this.value; }

      _set_properties(props?: Record<string, unknown>): void {
        this.properties = props;
        for (const l of this.listeners) l.properties_updated(this);
      }
      _set_values(values: Value[]): void {
        this.values = values;
        for (const l of this.listeners) l.values_updated(this);
      }
      _add_value(v: Value): void {
        this.value = v;
        this.values.push(v);
        for (const l of this.listeners) l.new_value(this, v);
      }

      add_item_listener(l: ItemListener): void { this.listeners.add(l); }
      remove_item_listener(l: ItemListener): void { this.listeners.delete(l); }

      to_string(): string { return this.properties && 'name' in this.properties ? this.properties.name as string : this.id; }
    }

    export interface ItemListener {

      properties_updated(item: Item): void;
      values_updated(item: Item): void;
      new_value(item: Item, v: Value): void;
    }
  }
}

type UpdateCoCoMessage = CoCoMessage | NewTypeMessage | NewItemMessage | NewDataMessage;

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

interface NewDataMessage extends ValueMessage {
  id: string;
}

interface PropMessage<V> {
  type: string;
  default_value?: V;
}

interface BoolPropertyMessage extends PropMessage<boolean> {
}

interface IntPropertyMessage extends PropMessage<number> {
  min?: number;
  max?: number;
}

interface FloatPropertyMessage extends PropMessage<number> {
  min?: number;
  max?: number;
}

interface StringPropertyMessage extends PropMessage<string> {
}

interface SymbolPropertyMessage extends PropMessage<string | string[]> {
  multiple: boolean;
  symbols?: string[];
}

interface ItemPropertyMessage extends PropMessage<string | string[]> {
  domain: string;
  multiple: boolean;
  items?: string[];
}

interface JSONPropertyMessage extends PropMessage<Record<string, any>> {
  schema: Record<string, any>;
}

type PropertyMessage = BoolPropertyMessage | IntPropertyMessage | FloatPropertyMessage | StringPropertyMessage | SymbolPropertyMessage | ItemPropertyMessage | JSONPropertyMessage;

interface TypeMessage {
  parents?: string[];
  data?: Record<string, any>;
  static_properties?: Record<string, PropertyMessage>;
  dynamic_properties?: Record<string, PropertyMessage>;
}

interface ValueMessage {
  data: Record<string, unknown>;
  timestamp: number;
}

interface ItemMessage {
  type: string;
  properties?: Record<string, unknown>;
  value?: ValueMessage;
}
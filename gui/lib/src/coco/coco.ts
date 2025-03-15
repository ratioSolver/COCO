export namespace coco {

  export class CoCo implements CoCoListener {

    private static instance: CoCo;
    private property_types = new Map<string, taxonomy.PropertyType<any>>();
    private types: Map<string, taxonomy.Type>;
    private items: Map<string, taxonomy.Item>;
    private coco_listeners: Set<CoCoListener> = new Set();

    private constructor() {
      this.add_property_type(new taxonomy.BoolPropertyType());
      this.add_property_type(new taxonomy.IntPropertyType());
      this.add_property_type(new taxonomy.FloatPropertyType());
      this.add_property_type(new taxonomy.StringPropertyType());
      this.add_property_type(new taxonomy.SymbolPropertyType());
      this.add_property_type(new taxonomy.ItemPropertyType());
      this.add_property_type(new taxonomy.JSONPropertyType());
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
          static_props.set(name, this.property_types.get(prop.type)!.make_property(this, prop));
        tp._set_static_properties(static_props);
      }
      if (tpm.dynamic_properties) {
        const dynamic_props = new Map<string, taxonomy.Property<unknown>>();
        for (const [name, prop] of Object.entries(tpm.dynamic_properties))
          dynamic_props.set(name, this.property_types.get(prop.type)!.make_property(this, prop));
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

      private name: string;

      constructor(name: string) { this.name = name; }

      get_name(): string { return this.name; }

      abstract make_property(cc: CoCo, property_message: PropertyMessage | any): P;
    }

    export class BoolPropertyType extends PropertyType<BoolProperty> {

      constructor() { super('bool'); }

      override make_property(_: CoCo, property_message: PropertyMessage | any): BoolProperty {
        const b_pm = property_message as BoolPropertyMessage;
        return new BoolProperty(b_pm.default_value);
      }
    }

    export class IntPropertyType extends PropertyType<IntProperty> {

      constructor() { super('int'); }

      override make_property(_: CoCo, property_message: PropertyMessage | any): IntProperty {
        const i_pm = property_message as IntPropertyMessage;
        return new IntProperty(i_pm.min, i_pm.max, i_pm.default_value);
      }
    }

    export class FloatPropertyType extends PropertyType<FloatProperty> {

      constructor() { super('float'); }

      override make_property(_: CoCo, property_message: PropertyMessage | any): FloatProperty {
        const f_pm = property_message as FloatPropertyMessage;
        return new FloatProperty(f_pm.min, f_pm.max, f_pm.default_value);
      }
    }

    export class StringPropertyType extends PropertyType<StringProperty> {

      constructor() { super('string'); }

      override make_property(_: CoCo, property_message: PropertyMessage | any): StringProperty {
        const s_pm = property_message as StringPropertyMessage;
        return new StringProperty(s_pm.default_value);
      }
    }

    export class SymbolPropertyType extends PropertyType<SymbolProperty> {

      constructor() { super('symbol'); }

      override make_property(_: CoCo, property_message: PropertyMessage | any): SymbolProperty {
        const sym_pm = property_message as SymbolPropertyMessage;
        return new SymbolProperty(sym_pm.multiple, sym_pm.symbols, sym_pm.default_value);
      }
    }

    export class ItemPropertyType extends PropertyType<ItemProperty> {

      constructor() { super('item'); }

      override make_property(cc: CoCo, property_message: PropertyMessage | any): ItemProperty {
        const itm_pm = property_message as ItemPropertyMessage;
        let def = undefined;
        if (itm_pm.default_value)
          def = Array.isArray(itm_pm.default_value) ? itm_pm.default_value.map(itm => cc.get_item(itm)) : cc.get_item(itm_pm.default_value);
        return new ItemProperty(cc.get_type(itm_pm.domain), itm_pm.multiple, itm_pm.items?.map(itm => cc.get_item(itm)), def);
      }
    }

    export class JSONPropertyType extends PropertyType<JSONProperty> {

      constructor() { super('json'); }

      override make_property(_: CoCo, property_message: PropertyMessage | any): JSONProperty {
        const j_pm = property_message as JSONPropertyMessage;
        return new JSONProperty(j_pm.schema, j_pm.default_value);
      }
    }

    export class Property<V> {

      private default_value: V | undefined;

      constructor(default_value?: V) {
        this.default_value = default_value;
      }

      to_string(): string { return this.default_value ? ` (${this.default_value})` : ''; }
    }

    export class BoolProperty extends Property<boolean> {
      constructor(default_value?: boolean) {
        super(default_value);
      }
    }

    export class IntProperty extends Property<number> {
      min?: number; max?: number;
      constructor(min?: number, max?: number, default_value?: number) {
        super(default_value);
        this.min = min;
        this.max = max;
      }
    }

    export class FloatProperty extends Property<number> {
      min?: number; max?: number;
      constructor(min?: number, max?: number, default_value?: number) {
        super(default_value);
        this.min = min;
        this.max = max;
      }
    }

    export class StringProperty extends Property<string> {
      constructor(default_value?: string) {
        super(default_value);
      }
    }

    export class SymbolProperty extends Property<string | string[]> {
      multiple: boolean;
      symbols?: string[];
      constructor(multiple: boolean, symbols?: string[], default_value?: string | string[]) {
        super(default_value);
        this.multiple = multiple;
        this.symbols = symbols;
      }
    }

    export class ItemProperty extends Property<Item | Item[]> {
      domain: Type;
      multiple: boolean;
      symbols?: Item[];
      constructor(domain: Type, multiple: boolean, symbols?: Item[], default_value?: Item | Item[]) {
        super(default_value);
        this.domain = domain;
        this.multiple = multiple;
        this.symbols = symbols;
      }
    }

    export class JSONProperty extends Property<Record<string, any>> {
      schema: Record<string, any>;
      constructor(schema: Record<string, any>, default_value?: Record<string, any>) {
        super(default_value);
        this.schema = schema;
      }
    }

    export class Type {

      private name: string;
      private parents?: Type[];
      private data?: Record<string, any>;
      private static_properties?: Map<string, Property<unknown>>;
      private dynamic_properties?: Map<string, Property<unknown>>;
      private listeners = new Set<TypeListener>();

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
      get_dynamic_properties(): Map<string, Property<unknown>> | undefined { return this.dynamic_properties; }

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

    export interface Value { data: Record<string, unknown>, timestamp: Date }

    export class Item {

      private id: string;
      private type: Type;
      private properties?: Record<string, unknown>;
      private value?: Value;
      private values: Value[] = [];
      private listeners = new Set<ItemListener>();

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
    }

    export interface ItemListener {

      properties_updated(item: Item): void;
      values_updated(item: Item): void;
      new_value(item: Item, v: Value): void;
    }
  }
}

type UpdateCoCoMessage = CoCoMessage | NewTypeMessage | NewItemMessage;

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
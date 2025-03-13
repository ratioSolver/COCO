export namespace coco {

  export class CoCo implements CoCoListener {

    private static instance: CoCo;
    _types: Map<string, taxonomy.Type>;
    _items: Map<string, taxonomy.Item>;
    private coco_listeners: Set<CoCoListener> = new Set();

    private constructor() {
      this._types = new Map();
      this._items = new Map();
    }

    static get_instance() {
      if (!CoCo.instance)
        CoCo.instance = new CoCo();
      return CoCo.instance;
    }

    new_type(type: taxonomy.Type): void {
      this._types.set(type.get_name(), type);
      for (const listener of this.coco_listeners) listener.new_type(type);
    }

    type_updated(type: taxonomy.Type): void { for (const listener of this.coco_listeners) listener.type_updated(type); }

    new_item(item: taxonomy.Item): void {
      this._items.set(item.get_id(), item);
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
          this.type_updated(n_tp);
          break;
        case 'new_item':
          const nim = message as NewItemMessage;
          this.new_item(this.make_item(nim.id, nim));
          break;
      }
    }

    private init(coco_message: CoCoMessage): void {
      if (coco_message.types) {
        this._types.clear();
        for (const [name, tp] of Object.entries(coco_message.types))
          this.new_type(new taxonomy.Type(name, [], tp.data));
        for (const [name, tp] of Object.entries(coco_message.types)) {
          const ctp = this._types.get(name)!;
          this.refine_type(ctp, tp);
          this.type_updated(ctp);
        }
      }
      if (coco_message.items) {
        this._items.clear();
        for (const [id, itm] of Object.entries(coco_message.items))
          this.new_item(this.make_item(id, itm));
      }
    }

    private refine_type(tp: taxonomy.Type, tpm: TypeMessage) {
      if (tpm.parents)
        tp._parents = tpm.parents.map(p => this._types.get(p)!);
      if (tpm.static_properties) {
        const static_props = new Map<string, taxonomy.Property<unknown>>();
        for (const [name, prop] of Object.entries(tpm.static_properties))
          static_props.set(name, taxonomy.make_property(this, prop));
        tp._static_properties = static_props;
      }
      if (tpm.dynamic_properties) {
        const dynamic_props = new Map<string, taxonomy.Property<unknown>>();
        for (const [name, prop] of Object.entries(tpm.dynamic_properties))
          dynamic_props.set(name, taxonomy.make_property(this, prop));
        tp._dynamic_properties = dynamic_props;
      }
    }

    private make_item(id: string, itm: ItemMessage): taxonomy.Item {
      let value = undefined;
      if (itm.value)
        value = { data: itm.value.data, timestamp: new Date(itm.value.timestamp) };
      return new taxonomy.Item(id, this._types.get(itm.type)!, itm.properties, value);
    }

    get_type(name: string): taxonomy.Type { return this._types.get(name)!; }
    get_item(id: string): taxonomy.Item { return this._items.get(id)!; }

    add_coco_listener(listener: CoCoListener) { this.coco_listeners.add(listener); }
    remove_coco_listener(listener: CoCoListener) { this.coco_listeners.delete(listener); }
  }

  export interface CoCoListener {

    new_type(type: taxonomy.Type): void;
    type_updated(type: taxonomy.Type): void;
    new_item(item: taxonomy.Item): void;
  }

  export namespace taxonomy {

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

    export function make_property(cc: CoCo, property_message: PropertyMessage): Property<unknown> {
      switch (property_message.type) {
        case 'bool':
          const b_pm = property_message as BoolPropertyMessage;
          return new BoolProperty(b_pm.default_value);
        case 'int':
          const i_pm = property_message as IntPropertyMessage;
          return new IntProperty(i_pm.min, i_pm.max, i_pm.default_value);
        case 'float':
          const f_pm = property_message as FloatPropertyMessage;
          return new FloatProperty(f_pm.min, f_pm.max, f_pm.default_value);
        case 'string':
          const s_pm = property_message as StringPropertyMessage;
          return new StringProperty(s_pm.default_value);
        case 'symbol':
          const sym_pm = property_message as SymbolPropertyMessage;
          return new SymbolProperty(sym_pm.multiple, sym_pm.symbols, sym_pm.default_value);
        case 'item':
          const itm_pm = property_message as ItemPropertyMessage;
          let def = undefined;
          if (itm_pm.default_value)
            def = Array.isArray(itm_pm.default_value) ? itm_pm.default_value.map(itm => cc.get_item(itm)) : cc.get_item(itm_pm.default_value);
          return new ItemProperty(cc.get_type(itm_pm.domain), itm_pm.multiple, itm_pm.items?.map(itm => cc.get_item(itm)), def);
        case 'json':
          const j_pm = property_message as JSONPropertyMessage;
          return new JSONProperty(j_pm.schema, j_pm.default_value);
        default:
          throw new Error(`Unknown property type: ${property_message.type}`);
      }
    }

    export class Type {

      private name: string;
      _parents?: Type[];
      private data?: Record<string, any>;
      _static_properties?: Map<string, Property<unknown>>;
      _dynamic_properties?: Map<string, Property<unknown>>;

      constructor(name: string, parents?: Type[], data?: Record<string, any>, static_properties?: Map<string, Property<unknown>>, dynamic_properties?: Map<string, Property<unknown>>) {
        this.name = name;
        this._parents = parents;
        this.data = data;
        this._static_properties = static_properties;
        this._dynamic_properties = dynamic_properties;
      }

      get_name(): string { return this.name; }
      get_parents(): Type[] | undefined { return this._parents; }
      get_data(): Record<string, any> | undefined { return this.data; }
      get_static_properties(): Map<string, Property<unknown>> | undefined { return this._static_properties; }
      get_dynamic_properties(): Map<string, Property<unknown>> | undefined { return this._dynamic_properties; }

      to_string(): string {
        let tp_str = `<html><b>${this.name}</b>`;
        if (this._static_properties) {
          tp_str += '<br><b>Static Properties:</b>';
          for (const [name, prop] of this._static_properties)
            tp_str += `<br><em>${name}</em> ${prop.to_string()}`;
        }
        if (this._dynamic_properties) {
          tp_str += '<br><b>Dynamic Properties:</b>';
          for (const [name, prop] of this._dynamic_properties)
            tp_str += `<br><em>${name}</em> ${prop.to_string()}`;
        }
        return tp_str + '</html>';
      }
    }

    export interface Value { data: Record<string, unknown>, timestamp: Date }

    export class Item {

      private id: string;
      private type: Type;
      private properties?: Record<string, unknown>;
      private value?: Value;

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
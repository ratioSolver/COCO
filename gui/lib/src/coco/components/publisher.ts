import { coco } from '../coco';

export namespace publisher {

  export class PublisherManager {

    private static instance: PublisherManager;
    private publishers: Map<string, PublisherGenerator<unknown>> = new Map();

    private constructor() {
      this.add_publisher_maker(new BoolPublisherGenerator());
      this.add_publisher_maker(new IntPublisherGenerator());
      this.add_publisher_maker(new FloatPublisherGenerator());
      this.add_publisher_maker(new StringPublisherGenerator());
      this.add_publisher_maker(new SymbolPublisherGenerator());
      this.add_publisher_maker(new ItemPublisherGenerator());
      this.add_publisher_maker(new JSONPublisherGenerator());
    }

    public static getInstance(): PublisherManager {
      if (!PublisherManager.instance)
        PublisherManager.instance = new PublisherManager();
      return PublisherManager.instance;
    }

    add_publisher_maker<V>(publisher: PublisherGenerator<V>) { this.publishers.set(publisher.get_name(), publisher); }
    get_publisher_maker(name: string): PublisherGenerator<unknown> { return this.publishers.get(name)!; }
  }

  export abstract class PublisherGenerator<V> {

    private name: string;

    constructor(name: string) {
      this.name = name;
    }

    get_name(): string { return this.name; }

    abstract make_publisher(name: string, property: coco.taxonomy.Property<V>, val: Record<string, undefined>): Publisher<V>;
  }

  export class BoolPublisherGenerator extends PublisherGenerator<boolean> {

    constructor() { super('bool'); }

    make_publisher(name: string, property: coco.taxonomy.BoolProperty, val: Record<string, undefined>): Publisher<boolean> { return new BoolPublisher(name, property, val); }
  }

  export class IntPublisherGenerator extends PublisherGenerator<number> {

    constructor() { super('int'); }

    make_publisher(name: string, property: coco.taxonomy.IntProperty, val: Record<string, undefined>): Publisher<number> { return new IntPublisher(name, property, val); }
  }

  export class FloatPublisherGenerator extends PublisherGenerator<number> {

    constructor() { super('float'); }

    make_publisher(name: string, property: coco.taxonomy.FloatProperty, val: Record<string, undefined>): Publisher<number> { return new FloatPublisher(name, property, val); }
  }

  export class StringPublisherGenerator extends PublisherGenerator<string> {

    constructor() { super('string'); }

    make_publisher(name: string, property: coco.taxonomy.StringProperty, val: Record<string, undefined>): Publisher<string> { return new StringPublisher(name, property, val); }
  }

  export class SymbolPublisherGenerator extends PublisherGenerator<string | string[]> {

    constructor() { super('symbol'); }

    make_publisher(name: string, property: coco.taxonomy.SymbolProperty, val: Record<string, undefined>): Publisher<string | string[]> { return new SymbolPublisher(name, property, val); }
  }

  export class ItemPublisherGenerator extends PublisherGenerator<coco.taxonomy.Item | coco.taxonomy.Item[]> {

    constructor() { super('item'); }

    make_publisher(name: string, property: coco.taxonomy.ItemProperty, val: Record<string, undefined>): Publisher<coco.taxonomy.Item | coco.taxonomy.Item[]> { return new ItemPublisher(name, property, val); }
  }

  export class JSONPublisherGenerator extends PublisherGenerator<Record<string, unknown>> {

    constructor() { super('json'); }

    make_publisher(name: string, property: coco.taxonomy.JSONProperty, val: Record<string, undefined>): Publisher<Record<string, unknown>> { return new JSONPublisher(name, property, val); }
  }

  export interface Publisher<V> {

    get_value(): V;
    set_value(v: V): void;
    get_element(): HTMLElement;
  }

  export class BoolPublisher implements Publisher<boolean> {

    private name: string;
    private val: Record<string, boolean | undefined>;
    private element: HTMLDivElement;

    constructor(name: string, property: coco.taxonomy.BoolProperty, val: Record<string, undefined>) {
      this.name = name;
      this.val = val;
      this.element = document.createElement('div');
      this.element.classList.add('form-check');
      const input = document.createElement('input');
      input.classList.add('form-check-input');
      input.type = 'checkbox';
      input.id = name;
      if (property.has_default_value())
        input.checked = property.get_default_value()!;
      input.addEventListener('change', () => this.val[this.name] = input.checked);
      this.element.append(input);
    }

    get_value(): boolean { return this.val[this.name]!; }
    set_value(v: boolean): void { this.val[this.name] = v; }
    get_element(): HTMLElement { return this.element; }
  }

  export class IntPublisher implements Publisher<number> {

    private name: string;
    private val: Record<string, number | undefined>;
    private element: HTMLDivElement;

    constructor(name: string, property: coco.taxonomy.IntProperty, val: Record<string, undefined>) {
      this.name = name;
      this.val = val;
      this.element = document.createElement('div');
      this.element.classList.add('form-check');
      const input = document.createElement('input');
      input.classList.add('form-check-input');
      input.type = 'number';
      input.id = name;
      input.step = '1';
      if (property.has_default_value())
        input.value = property.get_default_value()!.toString();
      if (property.get_min() !== undefined)
        input.min = property.get_min()!.toString();
      if (property.get_max() !== undefined)
        input.max = property.get_max()!.toString();
      input.addEventListener('change', () => this.val[this.name] = parseInt(input.value));
      this.element.append(input);
    }

    get_value(): number { return this.val[this.name]!; }
    set_value(v: number): void { this.val[this.name] = v; }
    get_element(): HTMLElement { return this.element; }
  }

  export class FloatPublisher implements Publisher<number> {

    private name: string;
    private val: Record<string, number | undefined>;
    private element: HTMLDivElement;

    constructor(name: string, property: coco.taxonomy.FloatProperty, val: Record<string, undefined>) {
      this.name = name;
      this.val = val;
      this.element = document.createElement('div');
      this.element.classList.add('form-check');
      const input = document.createElement('input');
      input.classList.add('form-check-input');
      input.type = 'number';
      input.id = name;
      if (property.has_default_value())
        input.value = property.get_default_value()!.toString();
      if (property.get_min() !== undefined)
        input.min = property.get_min()!.toString();
      if (property.get_max() !== undefined)
        input.max = property.get_max()!.toString();
      input.addEventListener('change', () => this.val[this.name] = parseFloat(input.value));
      this.element.append(input);
    }

    get_value(): number { return this.val[this.name]!; }
    set_value(v: number): void { this.val[this.name] = v; }
    get_element(): HTMLElement { return this.element; }
  }

  export class StringPublisher implements Publisher<string> {

    private name: string;
    private val: Record<string, string | undefined>;
    private element: HTMLDivElement;

    constructor(name: string, property: coco.taxonomy.StringProperty, val: Record<string, undefined>) {
      this.name = name;
      this.val = val;
      this.element = document.createElement('div');
      this.element.classList.add('form-check');
      const input = document.createElement('input');
      input.classList.add('form-check-input');
      input.type = 'text';
      input.id = name;
      if (property.has_default_value())
        input.value = property.get_default_value()!;
      input.addEventListener('change', () => this.val[this.name] = input.value);
      this.element.append(input);
    }

    get_value(): string { return this.val[this.name]!; }
    set_value(v: string): void { this.val[this.name] = v; }
    get_element(): HTMLElement { return this.element; }
  }

  export class SymbolPublisher implements Publisher<string | string[]> {

    private name: string;
    private multiple: boolean;
    private val: Record<string, string | string[] | undefined>;
    private element: HTMLDivElement;

    constructor(name: string, property: coco.taxonomy.SymbolProperty, val: Record<string, undefined>) {
      this.name = name;
      this.multiple = property.is_multiple();
      this.val = val;
      this.element = document.createElement('div');
      this.element.classList.add('form-check');
      const select = document.createElement('select');
      select.classList.add('form-select');
      select.id = name;
      if (this.multiple)
        select.multiple = true;
      if (property.get_symbols())
        property.get_symbols()!.forEach(s => {
          const option = document.createElement('option');
          option.value = s;
          option.text = s;
          select.append(option);
        });
      if (property.has_default_value())
        if (this.multiple)
          for (const v of property.get_default_value()! as string[])
            (select.querySelector(`option[value="${v}"]`) as HTMLOptionElement).selected = true;
        else
          select.value = property.get_default_value()! as string;
      select.addEventListener('change', () => {
        if (this.multiple)
          this.val[this.name] = Array.from(select.selectedOptions).map(o => o.value);
        else
          this.val[this.name] = select.value;
      }
      );
      this.element.append(select);
    }

    get_value(): string | string[] { return this.val[this.name]!; }
    set_value(v: string | string[]): void { this.val[this.name] = v; }
    get_element(): HTMLElement { return this.element; }
  }

  export class ItemPublisher implements Publisher<coco.taxonomy.Item | coco.taxonomy.Item[]> {

    private name: string;
    private multiple: boolean;
    private val: Record<string, string | string[] | undefined>;
    private element: HTMLDivElement;

    constructor(name: string, property: coco.taxonomy.ItemProperty, val: Record<string, undefined>) {
      this.name = name;
      this.multiple = property.is_multiple();
      this.val = val;
      this.element = document.createElement('div');
      this.element.classList.add('form-check');
      const select = document.createElement('select');
      select.classList.add('form-select');
      select.id = name;
      if (this.multiple)
        select.multiple = true;
      if (property.get_domain().get_instances())
        for (const i of property.get_domain().get_instances()) {
          const option = document.createElement('option');
          option.value = i.get_id();
          option.text = i.to_string();
          select.append(option);
        }
      if (property.has_default_value())
        if (this.multiple)
          for (const v of property.get_default_value()! as coco.taxonomy.Item[])
            (select.querySelector(`option[value="${v.get_id()}"]`) as HTMLOptionElement).selected = true
        else
          select.value = (property.get_default_value()! as coco.taxonomy.Item).get_id();
      select.addEventListener('change', () => {
        if (this.multiple)
          this.val[this.name] = Array.from(select.selectedOptions).map(o => o.value);
        else
          this.val[this.name] = select.value;
      }
      );
      this.element.append(select);
    }

    get_value(): coco.taxonomy.Item | coco.taxonomy.Item[] {
      if (this.multiple)
        return (this.val[this.name] as string[]).map(id => coco.CoCo.get_instance().get_item(id)!);
      else
        return coco.CoCo.get_instance().get_item(this.val[this.name] as string)!;
    }
    set_value(v: coco.taxonomy.Item | coco.taxonomy.Item[]): void {
      if (this.multiple)
        this.val[this.name] = (v as coco.taxonomy.Item[]).map(i => i.get_id());
      else
        this.val[this.name] = (v as coco.taxonomy.Item).get_id();
    }
    get_element(): HTMLElement { return this.element; }
  }

  export class JSONPublisher implements Publisher<Record<string, unknown>> {

    private name: string;
    private val: Record<string, Record<string, unknown> | undefined>;
    private element: HTMLDivElement;

    constructor(name: string, property: coco.taxonomy.JSONProperty, val: Record<string, undefined>) {
      this.name = name;
      this.val = val;
      this.element = document.createElement('div');
      this.element.classList.add('form-check');
      const textarea = document.createElement('textarea');
      textarea.classList.add('form-control');
      textarea.id = name;
      if (property.has_default_value())
        textarea.value = JSON.stringify(property.get_default_value()!);
      textarea.addEventListener('change', () => this.val[this.name] = JSON.parse(textarea.value));
      this.element.append(textarea);
    }

    get_value(): Record<string, unknown> { return this.val[this.name]!; }
    set_value(v: Record<string, unknown>): void { this.val[this.name] = v; }
    get_element(): HTMLElement { return this.element; }
  }
}
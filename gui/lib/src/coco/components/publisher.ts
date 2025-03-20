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

    public static get_instance(): PublisherManager {
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

    abstract make_publisher(name: string, property: coco.taxonomy.Property<unknown>, val: Record<string, unknown>): Publisher<V>;
  }

  export class BoolPublisherGenerator extends PublisherGenerator<boolean> {

    constructor() { super('bool'); }

    make_publisher(name: string, property: coco.taxonomy.BoolProperty, val: Record<string, unknown>): BoolPublisher { return new BoolPublisher(name, property, val); }
  }

  export class IntPublisherGenerator extends PublisherGenerator<number> {

    constructor() { super('int'); }

    make_publisher(name: string, property: coco.taxonomy.IntProperty, val: Record<string, unknown>): IntPublisher { return new IntPublisher(name, property, val); }
  }

  export class FloatPublisherGenerator extends PublisherGenerator<number> {

    constructor() { super('float'); }

    make_publisher(name: string, property: coco.taxonomy.FloatProperty, val: Record<string, unknown>): FloatPublisher { return new FloatPublisher(name, property, val); }
  }

  export class StringPublisherGenerator extends PublisherGenerator<string> {

    constructor() { super('string'); }

    make_publisher(name: string, property: coco.taxonomy.StringProperty, val: Record<string, unknown>): StringPublisher { return new StringPublisher(name, property, val); }
  }

  export class SymbolPublisherGenerator extends PublisherGenerator<string | string[]> {

    constructor() { super('symbol'); }

    make_publisher(name: string, property: coco.taxonomy.SymbolProperty, val: Record<string, unknown>): SymbolPublisher { return new SymbolPublisher(name, property, val); }
  }

  export class ItemPublisherGenerator extends PublisherGenerator<string | string[]> {

    constructor() { super('item'); }

    make_publisher(name: string, property: coco.taxonomy.ItemProperty, val: Record<string, unknown>): ItemPublisher { return new ItemPublisher(name, property, val); }
  }

  export class JSONPublisherGenerator extends PublisherGenerator<Record<string, unknown>> {

    constructor() { super('json'); }

    make_publisher(name: string, property: coco.taxonomy.JSONProperty, val: Record<string, unknown>): JSONPublisher { return new JSONPublisher(name, property, val); }
  }

  export interface Publisher<V> {

    get_value(): V;
    set_value(v: V): void;
    get_element(): HTMLElement;
  }

  export class BoolPublisher implements Publisher<boolean> {

    private readonly name: string;
    private readonly val: Record<string, unknown>;
    private readonly element: HTMLDivElement;
    private readonly input: HTMLInputElement;

    constructor(name: string, property: coco.taxonomy.BoolProperty, val: Record<string, unknown>) {
      this.name = name;
      this.val = val;
      this.element = document.createElement('div');
      this.element.classList.add('input-group');
      this.input = document.createElement('input');
      this.input.classList.add('form-check-input');
      this.input.type = 'checkbox';
      this.input.id = name;
      if (val[name])
        this.set_value(val[name] as boolean);
      else if (property.has_default_value())
        this.input.checked = property.get_default_value()!;
      this.input.addEventListener('change', () => this.val[this.name] = this.input.checked);
      this.element.append(this.input);
    }

    get_value(): boolean { return this.val[this.name]! as boolean; }
    set_value(v: boolean): void {
      this.val[this.name] = v;
      this.input.checked = v;
    }
    get_element(): HTMLElement { return this.element; }
  }

  export class IntPublisher implements Publisher<number> {

    private readonly name: string;
    private readonly val: Record<string, unknown>;
    private readonly element: HTMLDivElement;
    private readonly input: HTMLInputElement;

    constructor(name: string, property: coco.taxonomy.IntProperty, val: Record<string, unknown>) {
      this.name = name;
      this.val = val;
      this.element = document.createElement('div');
      this.element.classList.add('input-group');
      this.input = document.createElement('input');
      this.input.classList.add('form-control');
      this.input.type = 'number';
      this.input.id = name;
      this.input.step = '1';
      if (val[name])
        this.set_value(val[name] as number);
      else if (property.has_default_value())
        this.set_value(property.get_default_value()!);
      if (property.get_min() !== undefined)
        this.input.min = property.get_min()!.toString();
      if (property.get_max() !== undefined)
        this.input.max = property.get_max()!.toString();
      this.input.addEventListener('change', () => this.val[this.name] = parseInt(this.input.value));
      this.element.append(this.input);
    }

    get_value(): number { return this.val[this.name]! as number; }
    set_value(v: number): void {
      this.val[this.name] = v;
      this.input.value = v.toString();
    }
    get_element(): HTMLElement { return this.element; }
  }

  export class FloatPublisher implements Publisher<number> {

    private readonly name: string;
    private readonly val: Record<string, unknown>;
    private readonly element: HTMLDivElement;
    private readonly input: HTMLInputElement;

    constructor(name: string, property: coco.taxonomy.FloatProperty, val: Record<string, unknown>) {
      this.name = name;
      this.val = val;
      this.element = document.createElement('div');
      this.element.classList.add('input-group');
      this.input = document.createElement('input');
      this.input.classList.add('form-control');
      this.input.type = 'number';
      this.input.id = name;
      if (val[name])
        this.set_value(val[name] as number);
      else if (property.has_default_value())
        this.set_value(property.get_default_value()!);
      if (property.get_min() !== undefined)
        this.input.min = property.get_min()!.toString();
      if (property.get_max() !== undefined)
        this.input.max = property.get_max()!.toString();
      this.input.addEventListener('change', () => this.val[this.name] = parseFloat(this.input.value));
      this.element.append(this.input);
    }

    get_value(): number { return this.val[this.name]! as number; }
    set_value(v: number): void {
      this.val[this.name] = v;
      this.input.value = v.toString();
    }
    get_element(): HTMLElement { return this.element; }
  }

  export class StringPublisher implements Publisher<string> {

    private readonly name: string;
    private readonly val: Record<string, unknown>;
    private readonly element: HTMLDivElement;
    private readonly input: HTMLInputElement;

    constructor(name: string, property: coco.taxonomy.StringProperty, val: Record<string, unknown>) {
      this.name = name;
      this.val = val;
      this.element = document.createElement('div');
      this.element.classList.add('input-group');
      this.input = document.createElement('input');
      this.input.classList.add('form-control');
      this.input.type = 'text';
      this.input.id = name;
      if (val[name])
        this.set_value(val[name] as string);
      else if (property.has_default_value())
        this.set_value(property.get_default_value()!);
      this.input.addEventListener('change', () => this.val[this.name] = this.input.value);
      this.element.append(this.input);
    }

    get_value(): string { return this.val[this.name]! as string; }
    set_value(v: string): void {
      this.val[this.name] = v;
      this.input.value = v;
    }
    get_element(): HTMLElement { return this.element; }
  }

  export class SymbolPublisher implements Publisher<string | string[]> {

    private readonly name: string;
    private readonly multiple: boolean;
    private readonly val: Record<string, unknown>;
    private readonly element: HTMLDivElement;
    private readonly select: HTMLSelectElement;

    constructor(name: string, property: coco.taxonomy.SymbolProperty, val: Record<string, unknown>) {
      this.name = name;
      this.multiple = property.is_multiple();
      this.val = val;
      this.element = document.createElement('div');
      this.element.classList.add('input-group');
      this.select = document.createElement('select');
      this.select.classList.add('form-select');
      this.select.id = name;
      if (this.multiple)
        this.select.multiple = true;
      if (property.get_symbols())
        property.get_symbols()!.forEach(s => {
          const option = document.createElement('option');
          option.value = s;
          option.text = s;
          this.select.append(option);
        });
      if (val[name])
        this.set_value(val[name] as string | string[]);
      else if (property.has_default_value())
        this.set_value(property.get_default_value()!);
      this.select.addEventListener('change', () => {
        if (this.multiple) {
          const selected: string[] = [];
          for (let i = 0; i < this.select.options.length; i++)
            if ((this.select.options[i] as HTMLOptionElement).selected)
              selected.push(this.select.options[i].value);
          this.val[this.name] = selected
        }
        else
          this.val[this.name] = this.select.value;
      }
      );
      this.element.append(this.select);
    }

    get_value(): string | string[] { return this.val[this.name]! as string | string[]; }
    set_value(v: string | string[]): void {
      this.val[this.name] = v;
      if (this.multiple)
        for (let i = 0; i < this.select.options.length; i++)
          (this.select.options[i] as HTMLOptionElement).selected = (v as string[]).includes(this.select.options[i].value);
      else
        this.select.value = v as string;
    }
    get_element(): HTMLElement { return this.element; }
  }

  export class ItemPublisher implements Publisher<string | string[]> {

    private readonly name: string;
    private readonly multiple: boolean;
    private readonly val: Record<string, unknown>;
    private readonly element: HTMLDivElement;
    private readonly select: HTMLSelectElement;

    constructor(name: string, property: coco.taxonomy.ItemProperty, val: Record<string, unknown>) {
      this.name = name;
      this.multiple = property.is_multiple();
      this.val = val;
      this.element = document.createElement('div');
      this.element.classList.add('input-group');
      this.select = document.createElement('select');
      this.select.classList.add('form-select');
      this.select.id = name;
      if (this.multiple)
        this.select.multiple = true;
      if (property.get_domain().get_instances())
        for (const i of property.get_domain().get_instances()) {
          const option = document.createElement('option');
          option.value = i.get_id();
          option.text = i.to_string();
          this.select.append(option);
        }
      if (val[name])
        this.set_value(val[name] as string | string[]);
      else if (property.has_default_value())
        this.set_value(property.is_multiple() ? (property.get_default_value()! as coco.taxonomy.Item[]).map(i => i.get_id()) : (property.get_default_value()! as coco.taxonomy.Item).get_id());
      this.select.addEventListener('change', () => {
        if (this.multiple) {
          const selected: string[] = [];
          for (let i = 0; i < this.select.options.length; i++)
            if ((this.select.options[i] as HTMLOptionElement).selected)
              selected.push(this.select.options[i].value);
          this.val[this.name] = selected
        }
        else
          this.val[this.name] = this.select.value;
      }
      );
      this.element.append(this.select);
    }

    get_value(): string | string[] { return this.val[this.name]! as string | string[]; }
    set_value(v: string | string[]): void {
      this.val[this.name] = v;
      if (this.multiple)
        for (let i = 0; i < this.select.options.length; i++)
          (this.select.options[i] as HTMLOptionElement).selected = (v as string[]).includes(this.select.options[i].value);
      else
        this.select.value = v as string;
    }
    get_element(): HTMLElement { return this.element; }
  }

  export class JSONPublisher implements Publisher<Record<string, unknown>> {

    private readonly name: string;
    private readonly val: Record<string, unknown>;
    private readonly element: HTMLDivElement;
    private readonly textarea: HTMLTextAreaElement;

    constructor(name: string, property: coco.taxonomy.JSONProperty, val: Record<string, unknown>) {
      this.name = name;
      this.val = val;
      this.element = document.createElement('div');
      this.element.classList.add('input-group');
      this.textarea = document.createElement('textarea');
      this.textarea.classList.add('form-control');
      this.textarea.id = name;
      if (val[name])
        this.set_value(val[name] as Record<string, unknown>);
      else if (property.has_default_value())
        this.set_value(property.get_default_value()!);
      this.textarea.addEventListener('change', () => this.val[this.name] = JSON.parse(this.textarea.value));
      this.element.append(this.textarea);
    }

    get_value(): Record<string, unknown> { return this.val[this.name]! as Record<string, unknown>; }
    set_value(v: Record<string, unknown>): void {
      this.val[this.name] = v;
      this.textarea.value = JSON.stringify(v);
    }
    get_element(): HTMLElement { return this.element; }
  }
}
import { coco } from '../coco';

export namespace publisher {

  export class PublisherManager {

    private static instance: PublisherManager;
    private publishers: Map<string, PublisherGenerator<unknown>> = new Map();

    private constructor() {
      this.add_publisher_generator(new BoolPublisherGenerator());
      this.add_publisher_generator(new IntPublisherGenerator());
      this.add_publisher_generator(new FloatPublisherGenerator());
      this.add_publisher_generator(new StringPublisherGenerator());
      this.add_publisher_generator(new SymbolPublisherGenerator());
      this.add_publisher_generator(new ItemPublisherGenerator());
      this.add_publisher_generator(new JSONPublisherGenerator());
    }

    public static get_instance(): PublisherManager {
      if (!PublisherManager.instance)
        PublisherManager.instance = new PublisherManager();
      return PublisherManager.instance;
    }

    add_publisher_generator<V>(publisher: PublisherGenerator<V>) { this.publishers.set(publisher.get_name(), publisher); }
    get_publisher_generator(name: string): PublisherGenerator<unknown> { return this.publishers.get(name)!; }
  }

  export abstract class PublisherGenerator<V> {

    private name: string;

    constructor(name: string) {
      this.name = name;
    }

    get_name(): string { return this.name; }

    abstract make_publisher(name: string, property: coco.taxonomy.Property<unknown>, val: Record<string, unknown>): Publisher<V>;
  }

  class BoolPublisherGenerator extends PublisherGenerator<boolean | null> {

    constructor() { super('bool'); }

    make_publisher(name: string, property: coco.taxonomy.BoolProperty, val: Record<string, unknown>): BoolPublisher { return new BoolPublisher(name, property, val); }
  }

  class IntPublisherGenerator extends PublisherGenerator<number | null> {

    constructor() { super('int'); }

    make_publisher(name: string, property: coco.taxonomy.IntProperty, val: Record<string, unknown>): IntPublisher { return new IntPublisher(name, property, val); }
  }

  class FloatPublisherGenerator extends PublisherGenerator<number | null> {

    constructor() { super('float'); }

    make_publisher(name: string, property: coco.taxonomy.FloatProperty, val: Record<string, unknown>): FloatPublisher { return new FloatPublisher(name, property, val); }
  }

  class StringPublisherGenerator extends PublisherGenerator<string | null> {

    constructor() { super('string'); }

    make_publisher(name: string, property: coco.taxonomy.StringProperty, val: Record<string, unknown>): StringPublisher { return new StringPublisher(name, property, val); }
  }

  class SymbolPublisherGenerator extends PublisherGenerator<string | string[] | null> {

    constructor() { super('symbol'); }

    make_publisher(name: string, property: coco.taxonomy.SymbolProperty, val: Record<string, unknown>): SymbolPublisher { return new SymbolPublisher(name, property, val); }
  }

  class ItemPublisherGenerator extends PublisherGenerator<string | string[] | null> {

    constructor() { super('item'); }

    make_publisher(name: string, property: coco.taxonomy.ItemProperty, val: Record<string, unknown>): ItemPublisher { return new ItemPublisher(name, property, val); }
  }

  class JSONPublisherGenerator extends PublisherGenerator<Record<string, unknown> | null> {

    constructor() { super('json'); }

    make_publisher(name: string, property: coco.taxonomy.JSONProperty, val: Record<string, unknown>): JSONPublisher { return new JSONPublisher(name, property, val); }
  }

  export interface Publisher<V> {

    get_value(): V;
    set_value(v: V): void;
    get_element(): HTMLElement;
  }

  class BoolPublisher implements Publisher<boolean | null> {

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
        this.set_value(val[name] as boolean | null);
      else if (property.has_default_value())
        this.input.checked = property.get_default_value()!;
      this.input.addEventListener('change', () => this.val[this.name] = this.input.checked);
      this.element.append(this.input);
    }

    get_value(): boolean | null { return this.val[this.name]! as boolean | null; }
    set_value(v: boolean | null): void {
      this.val[this.name] = v;
      if (v === null)
        this.input.indeterminate = true;
      else {
        this.input.indeterminate = false;
        this.input.checked = v;
      }
    }
    get_element(): HTMLElement { return this.element; }
  }

  class IntPublisher implements Publisher<number | null> {

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
        this.set_value(val[name] as number | null);
      else if (property.has_default_value())
        this.set_value(property.get_default_value()!);
      this.input.min = property.get_min().toString();
      this.input.max = property.get_max().toString();
      this.input.addEventListener('change', () => this.val[this.name] = parseInt(this.input.value));
      this.element.append(this.input);
    }

    get_value(): number | null { return this.val[this.name]! as number | null; }
    set_value(v: number | null): void {
      this.val[this.name] = v;
      if (v)
        this.input.value = v.toString();
      else
        this.input.value = '';
    }
    get_element(): HTMLElement { return this.element; }
  }

  class FloatPublisher implements Publisher<number | null> {

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
      this.input.step = '0.01';
      this.input.id = name;
      if (val[name])
        this.set_value(val[name] as number | null);
      else if (property.has_default_value())
        this.set_value(property.get_default_value()!);
      this.input.min = property.get_min().toString();
      this.input.max = property.get_max().toString();
      this.input.addEventListener('change', () => this.val[this.name] = parseFloat(this.input.value));
      this.element.append(this.input);
    }

    get_value(): number | null { return this.val[this.name]! as number | null; }
    set_value(v: number | null): void {
      this.val[this.name] = v;
      if (v)
        this.input.value = v.toString();
      else
        this.input.value = '';
    }
    get_element(): HTMLElement { return this.element; }
  }

  class StringPublisher implements Publisher<string | null> {

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
        this.set_value(val[name] as string | null);
      else if (property.has_default_value())
        this.set_value(property.get_default_value()!);
      this.input.addEventListener('change', () => this.val[this.name] = this.input.value);
      this.element.append(this.input);
    }

    get_value(): string | null { return this.val[this.name]! as string | null; }
    set_value(v: string | null): void {
      if (v) {
        this.val[this.name] = v;
        this.input.value = v;
      } else {
        delete this.val[this.name];
        this.input.value = '';
      }
    }
    get_element(): HTMLElement { return this.element; }
  }

  class SymbolPublisher implements Publisher<string | string[] | null> {

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

      if (!this.multiple) {
        const placeholder = document.createElement('option');
        placeholder.value = '';
        placeholder.text = '';
        this.select.append(placeholder);
      }

      if (property.get_symbols())
        property.get_symbols()!.forEach(s => {
          const option = document.createElement('option');
          option.value = s;
          option.text = s;
          this.select.append(option);
        });
      if (val[name])
        this.set_value(val[name] as string | string[] | null);
      else if (property.has_default_value())
        this.set_value(property.get_default_value()!);
      this.select.addEventListener('change', () => {
        if (this.multiple) {
          const selected: string[] = [];
          for (let i = 0; i < this.select.options.length; i++)
            if ((this.select.options[i] as HTMLOptionElement).selected)
              selected.push(this.select.options[i].value);
          this.val[this.name] = selected;
        }
        else if (this.select.value !== '')
          this.val[this.name] = this.select.value;
        else
          this.val[this.name] = undefined;
      }
      );
      this.element.append(this.select);
    }

    get_value(): string | string[] | null { return this.val[this.name]! as string | string[] | null; }
    set_value(v: string | string[] | null): void {
      if (v) {
        this.val[this.name] = v;
        if (this.multiple)
          for (let i = 0; i < this.select.options.length; i++)
            (this.select.options[i] as HTMLOptionElement).selected = (v as string[]).includes(this.select.options[i].value);
        else
          this.select.value = v as string;
      } else {
        delete this.val[this.name];
        if (this.multiple) {
          for (let i = 0; i < this.select.options.length; i++)
            (this.select.options[i] as HTMLOptionElement).selected = false;
        }
        else
          this.select.value = '';
      }
    }
    get_element(): HTMLElement { return this.element; }
  }

  class ItemPublisher implements Publisher<string | string[] | null> {

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

      if (!this.multiple) {
        const placeholder = document.createElement('option');
        placeholder.value = '';
        placeholder.text = '';
        this.select.append(placeholder);
      }

      if (property.get_domain().get_instances())
        for (const i of property.get_domain().get_instances()) {
          const option = document.createElement('option');
          option.value = i.get_id();
          option.text = i.to_string();
          this.select.append(option);
        }
      if (val[name])
        this.set_value(val[name] as string | string[] | null);
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
        else if (this.select.value !== '')
          this.val[this.name] = this.select.value;
        else
          this.val[this.name] = undefined;
      }
      );
      this.element.append(this.select);
    }

    get_value(): string | string[] | null { return this.val[this.name]! as string | string[] | null; }
    set_value(v: string | string[] | null): void {
      if (v) {
        this.val[this.name] = v;
        if (this.multiple)
          for (let i = 0; i < this.select.options.length; i++)
            (this.select.options[i] as HTMLOptionElement).selected = (v as string[]).includes(this.select.options[i].value);
        else
          this.select.value = v as string;
      } else {
        delete this.val[this.name];
        if (this.multiple) {
          for (let i = 0; i < this.select.options.length; i++)
            (this.select.options[i] as HTMLOptionElement).selected = false;
        }
        else
          this.select.value = '';
      }
    }
    get_element(): HTMLElement { return this.element; }
  }

  class JSONPublisher implements Publisher<Record<string, unknown> | null> {

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
        this.set_value(val[name] as Record<string, unknown> | null);
      else if (property.has_default_value())
        this.set_value(property.get_default_value()!);
      this.textarea.addEventListener('change', () => this.val[this.name] = JSON.parse(this.textarea.value));
      this.element.append(this.textarea);
    }

    get_value(): Record<string, unknown> | null { return this.val[this.name]! as Record<string, unknown> | null; }
    set_value(v: Record<string, unknown> | null): void {
      if (v) {
        this.val[this.name] = v;
        this.textarea.value = JSON.stringify(v);
      } else {
        delete this.val[this.name];
        this.textarea.value = '';
      }
    }
    get_element(): HTMLElement { return this.element; }
  }
}
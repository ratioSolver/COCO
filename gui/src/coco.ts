export interface CoCoOptions {
  url?: string;
}

export namespace coco {
  export class CoCo {

    private readonly options: CoCoOptions;
    private readonly classes: Map<string, CoCoClass> = new Map();
    private readonly objects: Map<string, CoCoObject> = new Map();
    private socket: WebSocket | null = null;
    private readonly listeners: Set<CoCoListener> = new Set();

    constructor(options: CoCoOptions = {}) {
      this.options = {
        url: 'ws://' + window.location.host + '/ws',
        ...options
      };
    }

    connect() {
      if (this.socket)
        this.socket.close();

      console.log('Connecting to CoCo at', this.options.url);
      this.socket = new WebSocket(this.options.url!);
      this.socket.onopen = () => {
        console.log('CoCo connected');
        for (const listener of this.listeners) listener.connected();
      };
      this.socket.onclose = () => {
        console.log('CoCo disconnected');
        for (const listener of this.listeners) listener.disconnected();
      };
      this.socket.onerror = (error) => {
        console.error('CoCo connection error', error);
        for (const listener of this.listeners) listener.connection_error(error);
      };
      this.socket.onmessage = (event) => {
        console.trace('CoCo message received:', event.data);
        const msg: ServerMessage = JSON.parse(event.data);
        switch (msg.msg_type) {
          case 'coco': {
            for (const [name, cls] of Object.entries(msg.classes))
              this.classes.set(name, new CoCoClass(name, new Set(cls.parents || []), new Map(Object.entries(cls.static_properties || {})), new Map(Object.entries(cls.dynamic_properties || {}))));
            if (msg.objects)
              for (const [id, obj] of Object.entries(msg.objects))
                this.objects.set(id, new CoCoObject(id, new Set(obj.classes.map(cls_name => this.get_class(cls_name)))));
            for (const listener of this.listeners) listener.initialized();
            break;
          }
          case 'class_created': {
            const cls = new CoCoClass(msg.name, new Set(msg.parents || []), new Map(Object.entries(msg.static_properties || {})), new Map(Object.entries(msg.dynamic_properties || {})));
            this.classes.set(cls.get_name(), cls);
            for (const listener of this.listeners) listener.created_class(cls);
            break;
          }
          case 'object_created': {
            const obj = new CoCoObject(msg.id, new Set(msg.classes.map(cls_name => this.get_class(cls_name))));
            this.objects.set(obj.get_id(), obj);
            for (const listener of this.listeners) listener.created_object(obj);
            break;
          }
        }
      }
    }

    get_classes(): ReadonlyMap<string, CoCoClass> { return this.classes; }
    get_class(name: string): CoCoClass { return this.classes.get(name)!; }

    get_objects(): ReadonlyMap<string, CoCoObject> { return this.objects; }
    get_object(id: string): CoCoObject { return this.objects.get(id)!; }

    add_listener(listener: CoCoListener) { this.listeners.add(listener); }
    remove_listener(listener: CoCoListener) { this.listeners.delete(listener); }
  }

  export class CoCoClass {

    private readonly name: string;
    private readonly parents: Set<string>;
    private readonly static_properties: Map<string, Property>;
    private readonly dynamic_properties: Map<string, Property>;
    readonly _instances: Set<CoCoObject> = new Set();

    constructor(name: string, parents: Set<string> = new Set(), static_properties: Map<string, Property> = new Map(), dynamic_properties: Map<string, Property> = new Map()) {
      this.name = name;
      this.parents = parents;
      this.static_properties = static_properties;
      this.dynamic_properties = dynamic_properties;
    }

    get_name(): string { return this.name; }
    get_parents(): ReadonlySet<string> { return this.parents; }
    get_static_properties(): ReadonlyMap<string, Property> { return this.static_properties; }
    get_dynamic_properties(): ReadonlyMap<string, Property> { return this.dynamic_properties; }
    get_instances(): ReadonlySet<CoCoObject> { return this._instances; }
  }

  export class CoCoObject {

    private readonly id: string;
    private readonly classes: Set<CoCoClass>;
    private readonly properties?: Record<string, Value>;
    private readonly values?: Record<string, TimeValue>;

    constructor(id: string, classes: Set<CoCoClass>, properties?: Record<string, Value>, values?: Record<string, TimeValue>) {
      this.id = id;
      this.classes = classes;
      this.properties = properties;
      this.values = values;
      for (const cls of classes) cls._instances.add(this);
    }

    get_id(): string { return this.id; }
    get_classes(): ReadonlySet<CoCoClass> { return this.classes; }
    get_properties(): Record<string, Value> | undefined { return this.properties; }
    get_values(): Record<string, TimeValue> | undefined { return this.values; }
  }

  export interface CoCoListener {

    connected(): void;
    disconnected(): void;
    connection_error(error: Event): void;

    initialized(): void;
    created_class(cls: CoCoClass): void;
    created_object(obj: CoCoObject): void;
  }

  type Value = null | boolean | number | string;
  type TimeValue = [Value, string];

  type Property =
    | { type: 'bool', nullable?: boolean, default?: boolean }
    | { type: 'int', nullable?: boolean, default?: number, min?: number, max?: number }
    | { type: 'float', nullable?: boolean, default?: number, min?: number, max?: number }
    | { type: 'string', nullable?: boolean, default?: string }
    | { type: 'symbol', nullable?: boolean, default?: string, allowed_values?: string[] }
    | { type: 'object', nullable?: boolean, default?: string, class: string };

  type PartialClassMessage = {
    parents?: string[];
    static_properties?: Record<string, Property>;
    dynamic_properties?: Record<string, Property>;
  };
  type ClassMessage = ({ name: string } & PartialClassMessage);

  type PartialObjectMessage = { classes: string[], properties?: Record<string, Value>, values?: Record<string, TimeValue> };
  type ObjectMessage = ({ id: string } & PartialObjectMessage);

  type CoCoMessage = { classes: Record<string, PartialClassMessage>, objects?: Record<string, PartialObjectMessage> };

  type ServerMessage =
    | ({ msg_type: 'coco' } & CoCoMessage)
    | ({ msg_type: 'class_created' } & ClassMessage)
    | ({ msg_type: 'object_created' } & ObjectMessage)
    | ({ msg_type: 'added_class', object_id: string, class_name: string })
    | ({ msg_type: 'updated_properties', object_id: string, properties: Record<string, Value> })
    | ({ msg_type: 'added_values', object_id: string, values: Record<string, Value>, date_time: string });
}
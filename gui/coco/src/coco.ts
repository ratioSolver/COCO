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
        url: 'ws://localhost:3000/ws',
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
        const msg: ServerMessage = JSON.parse(event.data);
        switch (msg.msg_type) {
          case 'class_added': {
            const cls = new CoCoClass(msg.name);
            this.classes.set(cls.get_name(), cls);
            for (const listener of this.listeners) listener.added_class(cls);
            break;
          }
          case 'object_added': {
            const obj = new CoCoObject(msg.id);
            this.objects.set(obj.get_id(), obj);
            for (const listener of this.listeners) listener.added_object(obj);
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

    constructor(name: string) {
      this.name = name;
    }

    get_name(): string { return this.name; }
  }

  export class CoCoObject {

    private readonly id: string;
    private readonly properties?: Record<string, unknown>;

    constructor(id: string) {
      this.id = id;
    }

    get_id(): string { return this.id; }
    get_properties(): Record<string, unknown> | undefined { return this.properties; }
  }

  export interface CoCoListener {

    connected(): void;
    disconnected(): void;
    connection_error(error: Event): void;

    added_class(cls: CoCoClass): void;
    added_object(obj: CoCoObject): void;
  }

  type ClassMessage = { name: string };

  type ObjectMessage = { id: string; properties?: Record<string, unknown> };

  type ServerMessage =
    | ({ msg_type: 'class_added' } & ClassMessage)
    | ({ msg_type: 'object_added' } & ObjectMessage);
}
export interface CoCoOptions {
  url?: string;
}

export class CoCo {

  private readonly options: CoCoOptions;
  private readonly types: Map<string, Type> = new Map();
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
  }

  get_types(): Map<string, Type> { return this.types; }
  get_type(name: string): Type { return this.types.get(name)!; }

  add_listener(listener: CoCoListener) { this.listeners.add(listener); }
  remove_listener(listener: CoCoListener) { this.listeners.delete(listener); }
}

export class Type {

  private readonly name: string;

  constructor(name: string) {
    this.name = name;
  }

  get_name(): string { return this.name; }
}

export interface CoCoListener {

  connected(): void;
  disconnected(): void;
  connection_error(error: Event): void;
}
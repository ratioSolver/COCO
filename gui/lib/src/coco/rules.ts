export namespace rule {

  /**
   * Represents a reactive rule.
   */
  export class ReactiveRule {

    name: string;
    content: string;

    /**
     * Creates a new Rule instance.
     *
     * @param name The name of the rule.
     * @param content The content of the rule.
     */
    constructor(name: string, content: string) {
      this.name = name;
      this.content = content;
    }
  }

  /**
   * Represents a deliberative rule.
   */
  export class DeliberativeRule {

    name: string;
    content: string;

    /**
     * Creates a new Rule instance.
     *
     * @param name The name of the rule.
     * @param content The content of the rule.
     */
    constructor(name: string, content: string) {
      this.name = name;
      this.content = content;
    }
  }
}
#pragma once

#include "coco_module.hpp"
#include "coco_item.hpp"
#include "client.hpp"

namespace coco
{
  constexpr const char *intent_deftemplate = "(deftemplate intent (slot item_id (type SYMBOL)) (slot name (type SYMBOL)))";
  constexpr const char *entity_deftemplate = "(deftemplate entity (slot item_id (type SYMBOL)) (slot name (type SYMBOL)) (slot value))";
  constexpr const char *slot_deftemplate = "(deftemplate slot (slot item_id (type SYMBOL)) (slot name (type SYMBOL)) (slot value))";

  class intent;
  class entity;
  class slot;

  enum data_type
  {
    string_type,
    symbol_type,
    integer_type,
    float_type,
    boolean_type
  };
  inline data_type string_to_type(std::string_view type)
  {
    if (type == "string")
      return string_type;
    else if (type == "symbol")
      return symbol_type;
    else if (type == "int")
      return integer_type;
    else if (type == "float")
      return float_type;
    else if (type == "bool")
      return boolean_type;
    throw std::invalid_argument("Invalid data type");
  }

#ifdef BUILD_LISTENERS
  class llm_listener;
#endif

  class coco_llm final : public coco_module
  {
#ifdef BUILD_LISTENERS
    friend class llm_listener;
#endif
  public:
    coco_llm(coco &cc, std::string_view host = LLM_HOST, unsigned short port = LLM_PORT, std::string_view api_key = std::getenv("LLM_API_KEY")) noexcept;

    [[nodiscard]] std::vector<std::reference_wrapper<intent>> get_intents() noexcept;
    void create_intent(std::string_view name, std::string_view description, bool infere = true);
    [[nodiscard]] std::vector<std::reference_wrapper<entity>> get_entities() noexcept;
    void create_entity(data_type type, std::string_view name, std::string_view description, bool infere = true);
    [[nodiscard]] std::vector<std::reference_wrapper<slot>> get_slots() noexcept;
    void create_slot(data_type type, std::string_view name, std::string_view description, bool influence_context = true, bool infere = true);

    void set_slots(item &item, json::json &&props, bool infere = true) noexcept;
    void understand(item &item, std::string_view message, bool infere = true) noexcept;

  private:
    friend void set_slots_udf(Environment *env, UDFContext *udfc, UDFValue *out);
    friend void understand_udf(Environment *env, UDFContext *udfc, UDFValue *out);

#ifdef BUILD_LISTENERS
  private:
    void created_intent(const intent &i);
    void created_entity(const entity &e);
    void created_slot(const slot &s);
#endif

  private:
    std::map<std::string, std::unique_ptr<intent>, std::less<>> intents;  // The intents
    std::map<std::string, std::unique_ptr<entity>, std::less<>> entities; // The entities
    std::map<std::string, std::unique_ptr<slot>, std::less<>> slots;      // The slots
    std::map<std::string, std::map<std::string, Fact *>> slot_facts;      // The facts representing the slots for each item
    std::unordered_map<std::string, json::json> current_slots;            // The slots for each item
    network::ssl_client client;                                           // The client used to communicate with the LLM server
    const std::string api_key;                                            // The API key used to authenticate with the LLM server

#ifdef BUILD_LISTENERS
    std::vector<llm_listener *> listeners; // The LLM listeners..
#endif
  };

  class intent final
  {
  public:
    /**
     * @brief Constructs an intent with the given name and description.
     *
     * @param name The name of the intent.
     * @param description The description of the intent.
     */
    intent(std::string_view name, std::string_view description);

    /**
     * @brief Gets the name of the intent.
     *
     * @return The name of the intent.
     */
    [[nodiscard]] const std::string &get_name() const { return name; }
    /**
     * @brief Gets the description of the intent.
     *
     * @return The description of the intent.
     */
    [[nodiscard]] const std::string &get_description() const { return description; }

    [[nodiscard]] json::json to_json() const noexcept;

  private:
    std::string name;        // The name of the intent
    std::string description; // The description of the intent
  };

  class entity final
  {
  public:
    /**
     * @brief Constructs an entity with the given type, name, and description.
     *
     * @param type The type of the entity.
     * @param name The name of the entity.
     * @param description The description of the entity.
     */
    entity(data_type type, std::string_view name, std::string_view description);

    /**
     * @brief Gets the type of the entity.
     *
     * @return The type of the entity.
     */
    [[nodiscard]] data_type get_type() const { return type; }
    /**
     * @brief Gets the name of the entity.
     *
     * @return The name of the entity.
     */
    [[nodiscard]] const std::string &get_name() const { return name; }
    /**
     * @brief Gets the description of the entity.
     *
     * @return The description of the entity.
     */
    [[nodiscard]] const std::string &get_description() const { return description; }

    [[nodiscard]] json::json to_json() const noexcept;

  private:
    data_type type;          // The type of the entity
    std::string name;        // The name of the entity
    std::string description; // The description of the entity
  };

  class slot final
  {
  public:
    /**
     * @brief Constructs a slot with the given type, name, description, and influence context flag.
     *
     * @param type The type of the slot.
     * @param name The name of the slot.
     * @param description The description of the slot.
     * @param influence_context Indicates if the slot influences the context.
     */
    slot(data_type type, std::string_view name, std::string_view description, bool influence_context = true);
    /**
     * @brief Gets the type of the slot.
     *
     * @return The type of the slot.
     */
    [[nodiscard]] data_type get_type() const { return type; }
    /**
     * @brief Gets the name of the slot.
     *
     * @return The name of the slot.
     */
    [[nodiscard]] const std::string &get_name() const { return name; }
    /**
     * @brief Gets the description of the slot.
     *
     * @return The description of the slot.
     */
    [[nodiscard]] const std::string &get_description() const { return description; }
    /**
     * @brief Checks if the slot influences the context.
     *
     * @return True if the slot influences the context, false otherwise.
     */
    [[nodiscard]] bool influences_context() const { return influence_context; }

    [[nodiscard]] json::json to_json() const noexcept;

  private:
    data_type type;          // The type of the slot
    std::string name;        // The name of the slot
    std::string description; // The description of the slot
    bool influence_context;  // Indicates if the slot influences the context
  };

#ifdef BUILD_LISTENERS
  class llm_listener
  {
  public:
    llm_listener(coco_llm &llm) noexcept;
    virtual ~llm_listener();

    virtual void created_intent([[maybe_unused]] const intent &i) {}
    virtual void created_entity([[maybe_unused]] const entity &e) {}
    virtual void created_slot([[maybe_unused]] const slot &s) {}

  protected:
    coco_llm &llm; // The CoCo LLM object
  };
#endif

  /**
   * @brief Converts the entity type to a string representation.
   *
   * @param type The entity type to convert.
   * @return The string representation of the entity type.
   */
  [[nodiscard]] inline std::string type_to_string(data_type type)
  {
    switch (type)
    {
    case string_type:
      return "string";
    case symbol_type:
      return "symbol";
    case integer_type:
      return "integer";
    case float_type:
      return "float";
    case boolean_type:
      return "boolean";
    default:
      throw std::invalid_argument("Invalid entity type");
    }
  }

  void set_slots_udf(Environment *env, UDFContext *udfc, UDFValue *out);
  void understand_udf(Environment *env, UDFContext *udfc, UDFValue *out);
} // namespace coco

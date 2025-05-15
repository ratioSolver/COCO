#pragma once

#include "coco_module.hpp"
#include "coco_item.hpp"
#include "client.hpp"

namespace coco
{
  constexpr const char *intent_deftemplate = "(deftemplate intent (slot item_id (type SYMBOL)) (slot name (type SYMBOL)))";
  constexpr const char *entity_deftemplate = "(deftemplate entity (slot item_id (type SYMBOL)) (slot name (type SYMBOL)) (slot value))";

  class intent;
  class entity;

  enum entity_type
  {
    string_type,
    symbol_type,
    integer_type,
    float_type,
    boolean_type
  };

  class coco_llm final : public coco_module
  {
  public:
    coco_llm(coco &cc, std::string_view host = LLM_HOST, unsigned short port = LLM_PORT) noexcept;

    [[nodiscard]] std::vector<utils::ref_wrapper<intent>> get_intents() noexcept;
    void create_intent(std::string_view name, std::string_view description);
    [[nodiscard]] std::vector<utils::ref_wrapper<entity>> get_entities() noexcept;
    void create_entity(entity_type type, std::string_view name, std::string_view description, bool influence_context = true);

    void understand(item &item, std::string_view message) noexcept;

  private:
    friend void understand_udf(Environment *env, UDFContext *udfc, UDFValue *out);

  private:
    std::map<std::string, utils::u_ptr<intent>, std::less<>> intents;  // The intents
    std::map<std::string, utils::u_ptr<entity>, std::less<>> entities; // The entities
    std::unordered_map<std::string, json::json> slots;                 // The slots for a given item
    network::client client;                                            // The client used to communicate with the LLM server
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
     * @param influence_context Whether the entity influences the context.
     */
    entity(entity_type type, std::string_view name, std::string_view description, bool influence_context = true);

    /**
     * @brief Gets the type of the entity.
     *
     * @return The type of the entity.
     */
    [[nodiscard]] entity_type get_type() const { return type; }
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
    /**
     * @brief Checks if the entity influences the context.
     *
     * @return True if the entity influences the context, false otherwise.
     */
    [[nodiscard]] bool influences_context() const { return influence_context; }

  private:
    entity_type type;        // The type of the entity
    std::string name;        // The name of the entity
    std::string description; // The description of the entity
    bool influence_context;  // Whether the entity influences the context
  };

  class slot final
  {
  public:
    /**
     * @brief Constructs a slot with the given type, name, and description.
     *
     * @param type The type of the slot.
     * @param name The name of the slot.
     * @param description The description of the slot.
     */
    slot(entity_type type, std::string_view name, std::string_view description);
    /**
     * @brief Gets the type of the slot.
     *
     * @return The type of the slot.
     */
    [[nodiscard]] entity_type get_type() const { return type; }
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

  private:
    entity_type type;        // The type of the slot
    std::string name;        // The name of the slot
    std::string description; // The description of the slot
  };

  /**
   * @brief Converts the entity type to a string representation.
   *
   * @param type The entity type to convert.
   * @return The string representation of the entity type.
   */
  [[nodiscard]] inline std::string type_to_string(entity_type type)
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

  void understand_udf(Environment *env, UDFContext *udfc, UDFValue *out);
} // namespace coco

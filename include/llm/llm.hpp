#pragma once

#include "coco.hpp"
#include "client.hpp"

namespace coco
{
  constexpr const char *intent_deftemplate = "(deftemplate intent (slot item_id (type SYMBOL)) (slot name (type SYMBOL)))";
  constexpr const char *entity_deftemplate = "(deftemplate entity (slot item_id (type SYMBOL)) (slot name (type SYMBOL)) (slot value))";

  class intent final
  {
  public:
    /**
     * @brief Constructs an intent with the given name and description.
     *
     * @param name The name of the intent.
     * @param description The description of the intent.
     */
    intent(const std::string &name, const std::string &description) : name(name), description(description) {}

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

  enum entity_type
  {
    string_type,
    symbol_type,
    integer_type,
    float_type,
    boolean_type
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
    entity(entity_type type, const std::string &name, const std::string &description) : type(type), name(name), description(description) {}

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

  private:
    entity_type type;        // The type of the entity
    std::string name;        // The name of the entity
    std::string description; // The description of the entity
  };

  class llm final : public listener
  {
  public:
    llm(coco &cc, std::string_view host = LLM_HOST, unsigned short port = LLM_PORT) noexcept;

  private:
    friend void understand(Environment *env, UDFContext *udfc, UDFValue *out);

  private:
    network::client client;                           // The client used to communicate with the LLM server
    std::unordered_map<std::string, intent> intents;  // The intents
    std::unordered_map<std::string, entity> entities; // The entities
  };

  void understand(Environment *env, UDFContext *udfc, UDFValue *out);
} // namespace coco

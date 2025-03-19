#pragma once

#include "json.hpp"
#include "memory.hpp"
#include "clips.h"
#ifdef BUILD_EXECUTOR
#include "coco_executor.hpp"
#endif
#include <chrono>
#include <optional>
#include <unordered_map>
#include <mutex>
#include <random>

namespace coco
{
  constexpr const char *type_deftemplate = "(deftemplate type (slot name (type SYMBOL)))";
  constexpr const char *is_a_deftemplate = "(deftemplate is_a (slot type (type SYMBOL)) (slot parent (type SYMBOL)))";
  constexpr const char *item_deftemplate = "(deftemplate item (slot id (type SYMBOL)))";
  constexpr const char *instance_of_deftemplate = "(deftemplate instance_of (slot id (type SYMBOL)) (slot type (type SYMBOL)))";
  constexpr const char *inheritance_rule = "(defrule inheritance (is_a (type ?t) (parent ?p)) (instance_of (id ?i) (type ?t)) => (assert (instance_of (id ?i) (type ?p))))";
  constexpr const char *all_instances_of_function = "(deffunction all-instances-of (?type) (bind ?instances (create$)) (do-for-all-facts ((?instance_of instance_of)) (eq ?instance_of:type ?type) (bind ?instances (create$ ?instances ?instance_of:id))) (return ?instances))";
#ifdef BUILD_EXECUTOR
  constexpr const char *solver_deftemplate = "(deftemplate solver (slot name (type SYMBOL)) (slot state (allowed-values reasoning idle adapting executing finished failed)))";
  constexpr const char *task_deftemplate = "(deftemplate task (slot solver (type SYMBOL)) (slot id (type INTEGER)) (slot type (type SYMBOL)) (multislot pars (type SYMBOL)) (multislot vals) (slot since (type INTEGER) (default 0)))";
  constexpr const char *tick_function = "(deffunction tick (?year ?month ?day ?hour ?minute ?second) (do-for-all-facts ((?task task)) TRUE (modify ?task (since (+ ?task:since 1)))) (return TRUE))";
  constexpr const char *starting_function = "(deffunction starting (?solver ?task_type ?pars ?vals) (return TRUE))";
  constexpr const char *ending_function = "(deffunction ending (?solver ?id) (return TRUE))";
#endif

  class coco_db;
  class type;
  class item;
  class property_type;
  class property;
  class reactive_rule;
  class deliberative_rule;
#ifdef BUILD_LISTENERS
  class listener;
#endif

  class coco
  {
    friend class type;
    friend class item;
    friend class property;
    friend class reactive_rule;
#ifdef BUILD_EXECUTOR
    friend class coco_executor;
#endif
#ifdef BUILD_LISTENERS
    friend class listener;
#endif

  public:
    coco(coco_db &db) noexcept;
    ~coco();

    /**
     * @brief Returns a vector of references to the types.
     *
     * This function retrieves all the types stored in the database and returns them as a vector of `type` objects. The returned vector contains references to the actual types stored in the `types` map.
     *
     * @return A vector of types.
     */
    [[nodiscard]] std::vector<utils::ref_wrapper<type>> get_types() noexcept;

    /**
     * @brief Retrieves a type with the specified name.
     *
     * This function retrieves the type with the specified name.
     *
     * @param name The name of the type.
     * @return A reference to the type.
     * @throws std::invalid_argument if the type does not exist.
     */
    [[nodiscard]] type &get_type(std::string_view name);
    [[nodiscard]] type &create_type(std::string_view name, std::vector<utils::ref_wrapper<const type>> &&parents, json::json &&static_props, json::json &&dynamic_props, json::json &&data = json::json()) noexcept;
    void set_parents(type &tp, std::vector<utils::ref_wrapper<const type>> &&parents) noexcept;
    void delete_type(type &tp) noexcept;

    /**
     * @brief Returns a vector of references to the items.
     *
     * This function retrieves all the items stored in the database and returns them as a vector of `item` objects. The returned vector contains references to the actual items stored in the `items` map.
     *
     * @return A vector of items.
     */
    [[nodiscard]] std::vector<utils::ref_wrapper<item>> get_items() noexcept;

    [[nodiscard]] item &get_item(std::string_view id);
    [[nodiscard]] item &create_item(type &tp, json::json &&props = json::json(), std::optional<std::pair<json::json, std::chrono::system_clock::time_point>> &&val = std::nullopt) noexcept;
    void set_value(item &itm, json::json &&val, const std::chrono::system_clock::time_point &timestamp = std::chrono::system_clock::now());
    void delete_item(item &itm) noexcept;

    void create_reactive_rule(std::string_view rule_name, std::string_view rule_content);
    void create_deliberative_rule(std::string_view rule_name, std::string_view rule_content);

    [[nodiscard]] json::json to_json() const noexcept;

  protected:
    void add_property_type(utils::u_ptr<property_type> pt);

    [[nodiscard]] std::string to_string(Fact *f, std::size_t buff_size = 256) const noexcept;

  private:
    [[nodiscard]] property_type &get_property_type(std::string_view name) const;

    type &make_type(std::string_view name, std::vector<utils::ref_wrapper<const type>> &&parents, json::json &&static_props, json::json &&dynamic_props, json::json &&data = json::json());

    friend void add_data(Environment *env, UDFContext *udfc, UDFValue *out);

#ifdef BUILD_LISTENERS
  private:
#ifdef BUILD_EXECUTOR
    void state_changed(coco_executor &exec);

    void flaw_created(coco_executor &exec, const ratio::flaw &f);
    void flaw_state_changed(coco_executor &exec, const ratio::flaw &f);
    void flaw_cost_changed(coco_executor &exec, const ratio::flaw &f);
    void flaw_position_changed(coco_executor &exec, const ratio::flaw &f);
    void current_flaw(coco_executor &exec, std::optional<utils::ref_wrapper<ratio::flaw>> f);
    void resolver_created(coco_executor &exec, const ratio::resolver &r);
    void resolver_state_changed(coco_executor &exec, const ratio::resolver &r);
    void current_resolver(coco_executor &exec, std::optional<utils::ref_wrapper<ratio::resolver>> r);
    void causal_link_added(coco_executor &exec, const ratio::flaw &f, const ratio::resolver &r);

    void executor_state_changed(coco_executor &exec, ratio::executor::executor_state state);
    void tick(coco_executor &exec, const utils::rational &time);
    void starting(coco_executor &exec, const std::vector<utils::ref_wrapper<riddle::atom_term>> &atms);
    void start(coco_executor &exec, const std::vector<utils::ref_wrapper<riddle::atom_term>> &atms);
    void ending(coco_executor &exec, const std::vector<utils::ref_wrapper<riddle::atom_term>> &atms);
    void end(coco_executor &exec, const std::vector<utils::ref_wrapper<riddle::atom_term>> &atms);
#endif
    /**
     * @brief Notifies when the type is created.
     *
     * @param tp The created type.
     */
    void new_type(const type &tp) const;

    /**
     * @brief Notifies when the item is created.
     *
     * @param itm The created item.
     */
    void new_item(const item &itm) const;

    /**
     * @brief Notifies when the item is created.
     *
     * @param itm The created item.
     */
    void updated_item(const item &itm) const;

    /**
     * @brief Notifies when new data is added to the item.
     *
     * @param itm The item.
     * @param data The data.
     * @param timestamp The timestamp of the data.
     */
    void new_data(const item &itm, const json::json &data, const std::chrono::system_clock::time_point &timestamp) const;
#endif

  protected:
    coco_db &db;                                                                            // The database..
    json::json schemas;                                                                     // The JSON schemas..
    std::mt19937 gen;                                                                       // The random number generator..
    std::map<std::string, utils::u_ptr<property_type>, std::less<>> property_types;         // The property types..
    std::recursive_mutex mtx;                                                               // The mutex for the core..
    Environment *env;                                                                       // The CLIPS environment..
    std::map<std::string, utils::u_ptr<type>, std::less<>> types;                           // The types managed by CoCo by name.
    std::unordered_map<std::string, utils::u_ptr<item>> items;                              // The items by their ID..
    std::map<std::string, utils::u_ptr<reactive_rule>, std::less<>> reactive_rules;         // The reactive rules..
    std::map<std::string, utils::u_ptr<deliberative_rule>, std::less<>> deliberative_rules; // The deliberative rules..
#ifdef BUILD_EXECUTOR
    std::set<utils::u_ptr<coco_executor>> executors; // the executors..
#endif
#ifdef BUILD_LISTENERS
    std::vector<listener *> listeners; // The CoCo listeners..
#endif
  };

  void add_data(Environment *env, UDFContext *udfc, UDFValue *out);

#ifdef BUILD_LISTENERS
  class listener
  {
    friend class coco;

  public:
    listener(coco &cc) noexcept;
    virtual ~listener();

  private:
    /**
     * @brief Notifies when the type is created.
     *
     * @param tp The created type.
     */
    virtual void new_type([[maybe_unused]] const type &tp) {}

    /**
     * @brief Notifies when the item is created.
     *
     * @param itm The created item.
     */
    virtual void new_item([[maybe_unused]] const item &itm) {}

    /**
     * @brief Notifies when the item is updated.
     *
     * @param itm The updated item.
     */
    virtual void updated_item([[maybe_unused]] const item &itm) {}

    virtual void new_data([[maybe_unused]] const item &itm, [[maybe_unused]] const json::json &data, [[maybe_unused]] const std::chrono::system_clock::time_point &timestamp) {}

#ifdef BUILD_EXECUTOR
    virtual void state_changed([[maybe_unused]] coco_executor &exec) {}

    virtual void flaw_created([[maybe_unused]] coco_executor &exec, [[maybe_unused]] const ratio::flaw &f) {}
    virtual void flaw_state_changed([[maybe_unused]] coco_executor &exec, [[maybe_unused]] const ratio::flaw &f) {}
    virtual void flaw_cost_changed([[maybe_unused]] coco_executor &exec, [[maybe_unused]] const ratio::flaw &f) {}
    virtual void flaw_position_changed([[maybe_unused]] coco_executor &exec, [[maybe_unused]] const ratio::flaw &f) {}
    virtual void current_flaw([[maybe_unused]] coco_executor &exec, [[maybe_unused]] std::optional<utils::ref_wrapper<ratio::flaw>> f) {}
    virtual void resolver_created([[maybe_unused]] coco_executor &exec, [[maybe_unused]] const ratio::resolver &r) {}
    virtual void resolver_state_changed([[maybe_unused]] coco_executor &exec, [[maybe_unused]] const ratio::resolver &r) {}
    virtual void current_resolver([[maybe_unused]] coco_executor &exec, [[maybe_unused]] std::optional<utils::ref_wrapper<ratio::resolver>> r) {}
    virtual void causal_link_added([[maybe_unused]] coco_executor &exec, [[maybe_unused]] const ratio::flaw &f, [[maybe_unused]] const ratio::resolver &r) {}

    virtual void executor_state_changed([[maybe_unused]] coco_executor &exec, [[maybe_unused]] ratio::executor::executor_state state) {}
    virtual void tick([[maybe_unused]] coco_executor &exec, [[maybe_unused]] const utils::rational &time) {}
    virtual void starting([[maybe_unused]] coco_executor &exec, [[maybe_unused]] const std::vector<utils::ref_wrapper<riddle::atom_term>> &atms) {}
    virtual void start([[maybe_unused]] coco_executor &exec, [[maybe_unused]] const std::vector<utils::ref_wrapper<riddle::atom_term>> &atms) {}
    virtual void ending([[maybe_unused]] coco_executor &exec, [[maybe_unused]] const std::vector<utils::ref_wrapper<riddle::atom_term>> &atms) {}
    virtual void end([[maybe_unused]] coco_executor &exec, [[maybe_unused]] const std::vector<utils::ref_wrapper<riddle::atom_term>> &atms) {}
#endif
  protected:
    [[nodiscard]] std::recursive_mutex &get_mtx() const { return cc.mtx; }

    [[nodiscard]] Environment *get_env() const { return cc.env; }

    [[nodiscard]] std::string to_string(Fact *f, std::size_t buff_size = 256) const noexcept { return cc.to_string(f, buff_size); }

  protected:
    coco &cc;
  };
#endif
} // namespace coco
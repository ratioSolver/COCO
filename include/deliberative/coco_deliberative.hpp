#include "coco_module.hpp"
#include "plexa.hpp"
#ifdef BUILD_LISTENERS
#include "graph.hpp"
#endif

namespace coco
{
  constexpr const char *executor_deftemplate = "(deftemplate executor (slot name (type SYMBOL)) (slot state (allowed-values reasoning idle adapting executing finished failed)))";
  constexpr const char *task_deftemplate = "(deftemplate task (slot executor (type SYMBOL)) (slot id (type INTEGER)) (slot type (type SYMBOL)) (multislot pars (type SYMBOL)) (multislot vals) (slot since (type INTEGER) (default 0)))";
  constexpr const char *tick_function = "(deffunction tick (?year ?month ?day ?hour ?minute ?second) (do-for-all-facts ((?task task)) TRUE (modify ?task (since (+ ?task:since 1)))) (return TRUE))";
  constexpr const char *starting_function = "(deffunction starting (?executor ?task_type ?pars ?vals) (return TRUE))";
  constexpr const char *ending_function = "(deffunction ending (?executor ?id) (return TRUE))";

  class deliberative_rule;
  class coco_executor;
#ifdef BUILD_LISTENERS
  class deliberative_listener;
#endif

  class coco_deliberative : public coco_module
  {
    friend class coco_executor;
#ifdef BUILD_LISTENERS
    friend class deliberative_listener;
#endif
  public:
    coco_deliberative(coco &cc) noexcept;

    [[nodiscard]] std::vector<utils::ref_wrapper<deliberative_rule>> get_deliberative_rules() noexcept;
    void create_deliberative_rule(std::string_view rule_name, std::string_view rule_content);

    [[nodiscard]] coco_executor &create_executor(std::string_view name);
    void delete_executor(coco_executor &exec);

    friend void create_exec_script(Environment *env, UDFContext *udfc, UDFValue *out);
    friend void create_exec_rules(Environment *env, UDFContext *udfc, UDFValue *out);
    friend void start_execution(Environment *env, UDFContext *udfc, UDFValue *out);
    friend void delay_task(Environment *env, UDFContext *udfc, UDFValue *out);
    friend void extend_task(Environment *env, UDFContext *udfc, UDFValue *out);
    friend void failure(Environment *env, UDFContext *udfc, UDFValue *out);
    friend void adapt_script(Environment *env, UDFContext *udfc, UDFValue *out);
    friend void adapt_rules(Environment *env, UDFContext *udfc, UDFValue *out);
    friend void delete_exec(Environment *env, UDFContext *udfc, UDFValue *out);

#ifdef BUILD_LISTENERS
  private:
    void created_deliberative_rule(const deliberative_rule &rule);

    void created_executor(coco_executor &exec);
    void deleted_executor(coco_executor &exec);

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

  private:
    void to_json(json::json &j) const noexcept override;

  private:
    std::map<std::string, utils::u_ptr<deliberative_rule>, std::less<>> deliberative_rules; // The deliberative rules..
    std::unordered_map<std::string, utils::u_ptr<coco_executor>> executors;                 // the executors..

#ifdef BUILD_LISTENERS
  private:
    std::vector<deliberative_listener *> listeners; // The CoCo listeners..
#endif
  };

  /**
   * @brief Represents a deliberative CoCo rule.
   *
   * This class represents a deliberative CoCo rule in the form of a name and content.
   */
  class deliberative_rule final
  {
  public:
    /**
     * @brief Constructs a rule object.
     *
     * @param cd The CoCo core object.
     * @param name The name of the rule.
     * @param content The content of the rule.
     */
    deliberative_rule(coco_deliberative &cd, std::string_view name, std::string_view content) noexcept;

    /**
     * @brief Gets the name of the rule.
     *
     * @return The name of the rule.
     */
    [[nodiscard]] const std::string &get_name() const { return name; }

    /**
     * @brief Gets the content of the rule.
     *
     * @return The content of the rule.
     */
    [[nodiscard]] const std::string &get_content() const { return content; }

    [[nodiscard]] json::json to_json() const noexcept;

  private:
    coco_deliberative &cd; // the CoCo core object.
    std::string name;      // the name of the rule.
    std::string content;   // the content of the rule.
  };

#ifdef BUILD_LISTENERS
  class deliberative_listener
  {
    friend class coco_deliberative;

  public:
    deliberative_listener(coco_deliberative &cd) noexcept;
    virtual ~deliberative_listener();

  private:
    virtual void deliberative_rule_created([[maybe_unused]] const deliberative_rule &rule) {}

    virtual void executor_created([[maybe_unused]] coco_executor &exec) {}
    virtual void executor_deleted([[maybe_unused]] coco_executor &exec) {}

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

  protected:
    coco_deliberative &cd; // reference to the coco_deliberative object
  };
#endif

  void create_exec_script(Environment *env, UDFContext *udfc, UDFValue *out);
  void create_exec_rules(Environment *env, UDFContext *udfc, UDFValue *out);
  void start_execution(Environment *env, UDFContext *udfc, UDFValue *out);
  void delay_task(Environment *env, UDFContext *udfc, UDFValue *out);
  void extend_task(Environment *env, UDFContext *udfc, UDFValue *out);
  void failure(Environment *env, UDFContext *udfc, UDFValue *out);
  void adapt_script(Environment *env, UDFContext *udfc, UDFValue *out);
  void adapt_rules(Environment *env, UDFContext *udfc, UDFValue *out);
  void delete_exec(Environment *env, UDFContext *udfc, UDFValue *out);
} // namespace coco

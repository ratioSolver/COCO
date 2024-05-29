(deftemplate item_type (slot id (type SYMBOL)) (slot name (type STRING)) (slot description (type STRING)) (multislot static_parameters) (multislot dynamic_parameters))
(deftemplate is_subtype_of (slot parent (type SYMBOL)) (slot child (type SYMBOL)))
(deftemplate is_instance_of (slot item (type SYMBOL)) (slot item_type (type SYMBOL)))
(deftemplate item (slot id (type SYMBOL)) (slot name (type STRING)) (slot description (type STRING)) (multislot parameters))
(deffunction item_data (?item ?item_type ?time ?data))

(deftemplate solver (slot solver_ptr (type INTEGER)) (slot solver_type (type SYMBOL)) (slot state (allowed-values reasoning idle adapting executing finished failed)))
(deftemplate task (slot solver_ptr (type INTEGER)) (slot id (type INTEGER)) (slot task_type (type SYMBOL)) (multislot pars (type SYMBOL)) (multislot vals) (slot since (type INTEGER) (default 0)))

(defrule new_subtype
    (is_subtype_of (parent ?parent) (child ?child))
    (is_instance_of (item ?child) (item_type ?child_type))
    (not (is_instance_of (item ?parent) (item_type ?parent_type)))
    =>
    (assert (is_instance_of (item ?parent) (item_type ?child_type)))
)

(deffunction add_task (?solver_ptr ?id ?task_type ?pars ?vals)
    (assert (task (solver_ptr ?solver_ptr) (id ?id) (task_type ?task_type) (pars ?pars) (vals ?vals)))
    (return TRUE)
)

(deffunction remove_task (?solver_ptr ?id)
    (do-for-fact ((?task task)) (eq ?task:solver_ptr ?solver_ptr) (eq ?task:id ?id) (retract ?task))
    (return TRUE)
)

(deffunction tick ()
    (do-for-all-facts ((?task task)) TRUE (modify ?task (since (+ ?task:since 1))))
    (return TRUE)
)

(deffunction starting (?solver_ptr ?task_type ?pars ?vals) (return TRUE))
(deffunction ending (?solver_ptr ?id) (return TRUE))
(deffunction start (?solver_ptr ?id ?task_type ?pars ?vals)
    (add_task ?solver_ptr ?id ?task_type ?pars ?vals)
)
(deffunction end (?solver_ptr ?id)
    (remove_task ?solver_ptr ?id)
)

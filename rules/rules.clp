(deftemplate sensor_type (slot id (type SYMBOL)) (slot name (type STRING)) (slot description (type STRING)))
(deftemplate sensor (slot id (type SYMBOL)) (slot sensor_type (type SYMBOL)) (slot name (type STRING)) (slot description (type STRING)) (multislot location))

(deffunction sensor_data (?sensor ?sensor_type ?time ?data))
(deffunction sensor_state (?sensor ?sensor_type ?time ?state))

(deftemplate solver (slot solver_ptr (type INTEGER)) (slot solver_type (type SYMBOL)) (slot state (allowed-values reasoning idle adapting executing finished failed)))
(deftemplate task (slot solver_ptr (type INTEGER)) (slot id (type INTEGER)) (slot task_type (type SYMBOL)) (multislot pars (type SYMBOL)) (multislot vals) (slot since (type INTEGER) (default 0)))

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
    (return TRUE)
)
(deffunction end (?solver_ptr ?id)
    (remove_task ?solver_ptr ?id)
    (return TRUE)
)

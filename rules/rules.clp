(deftemplate configuration (slot network_ptr))
(deftemplate solver (slot solver_ptr) (slot solver_type))

(deftemplate task (slot command) (slot task_type) (multislot pars) (multislot vals))

(deftemplate sensor_type (slot id) (slot name) (slot description))
(deftemplate sensor (slot id) (slot sensor_type) (multislot location))

(defrule new_configuration
    (configuration (network_ptr ?ptr))
    =>
    (assert (solver (solver_ptr (new_solver ?ptr)) (solver_type main)))
)
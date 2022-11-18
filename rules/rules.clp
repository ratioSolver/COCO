(deftemplate configuration (slot network_ptr))
(deftemplate solver (slot solver_ptr) (slot solver_type))

(deftemplate task (slot command) (slot id) (slot task_type) (multislot pars) (multislot vals))
(deftemplate dont_start_yet (slot task_id) (multislot delay_time))
(deftemplate dont_end_yet (slot task_id) (multislot delay_time))

(deftemplate sensor_type (slot id) (slot name) (slot description))
(deftemplate sensor (slot id) (slot sensor_type) (multislot location))

(defrule new_configuration
    (configuration (network_ptr ?ptr))
    =>
    (assert (solver (solver_ptr (new_solver ?ptr)) (solver_type main)))
)
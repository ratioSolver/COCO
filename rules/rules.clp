(deftemplate configuration (slot coco_ptr))
(deftemplate solver (slot solver_ptr) (slot solver_type) (slot state))

(deftemplate task (slot command) (slot id) (slot task_type) (multislot pars) (multislot vals))
(deftemplate dont_start_yet (slot task_id) (multislot delay_time))
(deftemplate dont_end_yet (slot task_id) (multislot delay_time))

(deftemplate sensor_type (slot id) (slot name) (slot description))
(deftemplate sensor (slot id) (slot sensor_type) (slot name) (multislot location))

(defrule new_configuration
    (configuration (coco_ptr ?ptr))
    =>
    (new_solver_script ?ptr main "")
)
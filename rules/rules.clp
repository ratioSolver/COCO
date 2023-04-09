(deftemplate configuration (slot coco_ptr (type INTEGER)))
(deftemplate solver (slot solver_ptr (type INTEGER)) (slot solver_type (type SYMBOL)) (slot state (allowed-values reasoning idle adapting executing finished failed)))

(deftemplate task (slot solver_ptr (type INTEGER)) (slot command (allowed-values starting start ending end)) (slot id (type INTEGER)) (slot task_type (type SYMBOL)) (multislot pars) (multislot vals))
(deftemplate dont_start_yet (slot task_id (type INTEGER)) (multislot delay_time))
(deftemplate dont_end_yet (slot task_id (type INTEGER)) (multislot delay_time))

(deftemplate sensor_type (slot id) (slot name (type STRING)) (slot description (type STRING)))
(deftemplate sensor (slot id) (slot sensor_type) (slot name (type STRING)) (slot description (type STRING)) (multislot location))
(deftemplate sensor_state (slot sensor_id) (slot state))
(deftemplate sensor_data (slot sensor_id) (slot local_time) (multislot data))
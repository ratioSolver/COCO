(deftemplate solver (slot solver_ptr (type INTEGER)) (slot solver_type (type SYMBOL)) (slot state (allowed-values reasoning idle adapting executing finished failed)))

(deftemplate sensor_type (slot id) (slot name (type STRING)) (slot description (type STRING)))
(deftemplate sensor (slot id) (slot sensor_type) (slot name (type STRING)) (slot description (type STRING)) (multislot location))
(deftemplate sensor_state (slot sensor_id) (slot state))
(deftemplate sensor_data (slot sensor_id) (slot local_time) (multislot data))

(deffunction starting (?solver_ptr ?task_type ?pars ?vals) (return TRUE))
(deffunction ending (?solver_ptr ?id) (return TRUE))
(deffunction start (?solver_ptr ?id ?task_type ?pars ?vals) (println "Starting task" ?id "of type" ?task_type "with parameters" ?pars "and values" ?vals))
(deffunction end (?solver_ptr ?id) (println "Ending task of type" ?task_type "with parameters" ?pars "and values" ?vals))

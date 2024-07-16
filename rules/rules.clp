(deftemplate item_type (slot id (type SYMBOL)) (slot name (type STRING)) (slot description (type STRING)) (multislot static_parameters) (multislot dynamic_parameters))
(deftemplate item (slot id (type SYMBOL)) (slot name (type STRING)) (slot description (type STRING)) (multislot parameters))
(deftemplate is_instance_of (slot item_id (type SYMBOL)) (slot type_id (type SYMBOL)))
(deffunction item_data (?item ?item_type ?time ?data))

(deftemplate solver (slot id (type INTEGER)) (slot purpose (type SYMBOL)) (slot state (allowed-values reasoning idle adapting executing finished failed)))
(deftemplate task (slot solver_id (type INTEGER)) (slot id (type INTEGER)) (slot task_type (type SYMBOL)) (multislot pars (type SYMBOL)) (multislot vals) (slot since (type INTEGER) (default 0)))

(deffunction tick () (do-for-all-facts ((?task task)) TRUE (modify ?task (since (+ ?task:since 1)))) (return TRUE))

(deffunction starting (?id ?task_type ?pars ?vals) (return TRUE))
(deffunction ending (?id ?id) (return TRUE))

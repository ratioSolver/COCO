(deftemplate temperature (slot battery) (slot temp))

(defrule update_temp
    (solver (solver_type thermostat))
    (sensor_data (sensor_id ?id) (local_time ?t) (data ?b ?d))
    (sensor (id ?id) (sensor_type ?tp))
    (sensor_type (id ?tp) (name "temperature"))
    ?temp <- (temperature)
    =>
    (modify ?temp (battery ?b) (temp ?d))
)

(defrule cold_temp
    (configuration (coco_ptr ?cc))
    (not (solver (solver_type thermostat)))
    (sensor_data (sensor_id ?id) (local_time ?t) (data ?b ?d))
    (sensor (id ?id) (sensor_type ?tp))
    (sensor_type (id ?tp) (name "temperature"))
    (test (<= ?d 23))
    =>
    (assert (temperature (battery ?b) (temp ?d)))
    (new_solver_script ?cc thermostat "class Thermostat : StateVariable { predicate TurningOff() { duration >= 10.0; } predicate Heating() { duration >= 10.0; goal off = new TurningOff(); off.start >= end; } predicate Cooling() { duration >= 10.0; goal off = new TurningOff(); off.start >= end; } } Thermostat thermostat = new Thermostat(); goal heat = new thermostat.Heating(); heat.start >= 10.0;")
)

(defrule start_thermostat
    (solver (solver_type thermostat) (solver_ptr ?sp) (state idle))
    =>
    (start_execution ?sp)
)

(defrule delete_thermostat
    (solver (solver_type thermostat) (solver_ptr ?sp) (state finished))
    ?temp <- (temperature)
    =>
    (delete_solver ?sp)
    (retract ?temp)
)

(defrule continue_heating
    (task (command ending) (id ?task_id) (task_type Heating))
    (temperature (temp ?d))
    (test (<= ?d 23))
    =>
    (assert (dont_end_yet (task_id ?task_id)))
)
(defrule temp
    (configuration (coco_ptr ?cc))
    (not (solver (solver_type thermostat)))
    (sensor_data (sensor_id ?id) (local_time ?t) (data ?d))
    (sensor (id ?id) (sensor_type ?tp))
    (sensor_type (id ?tp) (name "temperature"))
    (test (<= ?d 25))
    =>
    (new_solver_script ?cc thermostat "class Thermostat : StateVariable { predicate On() { duration >= 10.0; } } Thermostat thermostat = new Thermostat(); goal on = new thermostat.On(); on.start >= 10.0;")
)
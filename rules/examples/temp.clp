(deftemplate temperature (slot battery) (slot temp))

(defrule update_temp
    (solver (solver_type thermostat))
    (sensor_data (sensor_id ?id) (data ?batt ?temp))
    (sensor (id ?id) (sensor_type ?tp))
    (sensor_type (id ?tp) (name "temperature"))
    ?temp_fact <- (temperature)
    =>
    (modify ?temp_fact (battery ?batt) (temp ?temp))
)

(defrule cold_temp
    (configuration (coco_ptr ?cc))
    (not (solver (solver_type thermostat)))
    (sensor_data (sensor_id ?id) (data ?batt ?temp))
    (sensor (id ?id) (sensor_type ?tp))
    (sensor_type (id ?tp) (name "temperature"))
    (test (<= ?temp 23))
    =>
    (printout t "cold temp" crlf)
    (assert (temperature (battery ?batt) (temp ?temp)))
    (new_solver_script ?cc thermostat (str-cat "class Thermostat : StateVariable { predicate Temperature(real temp) { false; } predicate Heating() { duration >= 10.0; goal temp = new Temperature(end: start); temp.temp <= 23.0; } predicate Cooling() { duration >= 10.0; goal temp = new Temperature(end: start); temp.temp >= 27.0; } predicate Comfort() { duration >= 10.0; { goal heat = new Heating(end: start); } or { goal cool = new Cooling(end: start); } } } Thermostat thermostat = new Thermostat(); fact temp = new thermostat.Temperature(start: 0.0, end: 10.0, temp: " (float ?temp) "); goal heat = new thermostat.Comfort();"))
)

(defrule hot_temp
    (configuration (coco_ptr ?cc))
    (not (solver (solver_type thermostat)))
    (sensor_data (sensor_id ?id) (data ?b ?temp))
    (sensor (id ?id) (sensor_type ?tp))
    (sensor_type (id ?tp) (name "temperature"))
    (test (>= ?temp 27))
    =>
    (printout t "hot temp" crlf)
    (assert (temperature (battery ?b) (temp ?temp)))
    (new_solver_script ?cc thermostat (str-cat "class Thermostat : StateVariable { predicate Temperature(real temp) { false; } predicate Heating() { duration >= 10.0; goal temp = new Temperature(end: start); temp.temp <= 23.0; } predicate Cooling() { duration >= 10.0; goal temp = new Temperature(end: start); temp.temp >= 27.0; } predicate Comfort() { duration >= 10.0; { goal heat = new Heating(end: start); } or { goal cool = new Cooling(end: start); } } } Thermostat thermostat = new Thermostat(); fact temp = new thermostat.Temperature(start: 0.0, end: 10.0, temp: " (float ?temp) "); goal heat = new thermostat.Comfort();"))
)

(defrule start_thermostat
    (solver (solver_type thermostat) (solver_ptr ?sp) (state idle))
    =>
    (start_execution ?sp)
)

(defrule delete_thermostat
    (solver (solver_type thermostat) (solver_ptr ?sp) (state finished))
    ?temp_fact <- (temperature)
    =>
    (delete_solver ?sp)
    (retract ?temp_fact)
)

(defrule continue_heating
    (task (command ending) (id ?task_id) (task_type Heating))
    (temperature (temp ?temp))
    (test (<= ?temp 23))
    =>
    (assert (dont_end_yet (task_id ?task_id)))
)

(defrule continue_cooling
    (task (command ending) (id ?task_id) (task_type Cooling))
    (temperature (temp ?temp))
    (test (>= ?temp 27))
    =>
    (assert (dont_end_yet (task_id ?task_id)))
)
(deftemplate blood_pressure (slot systolic_blood_pressure) (slot diastolic_blood_pressure) (slot pulse_rate))

(defrule update_blood_pressure
    (solver (solver_type health))
    (sensor_data (sensor_id ?id) (data ?dbp ?pr ?sbd))
    (sensor (id ?id) (sensor_type ?tp))
    (sensor_type (id ?tp) (name "blood_pressure"))
    ?bp <- (blood_pressure)
    =>
    (modify ?bp (systolic_blood_pressure ?sbd) (diastolic_blood_pressure ?dbp) (pulse_rate ?pr))
)

(defrule risky_blood_pressure
    (configuration (coco_ptr ?cc))
    (not (solver (solver_type health)))
    (sensor_data (sensor_id ?id) (data ?batt ?temp))
    (sensor (id ?id) (sensor_type ?tp))
    (sensor_type (id ?tp) (name "blood_pressure"))
    (or
        (and (>= ?sbd 140) (>= ?dbp 90))
        (>= ?pr 100)
    )
    =>
    (assert (blood_pressure (systolic_blood_pressure ?sbd) (diastolic_blood_pressure ?dbp) (pulse_rate ?pr)))
    (new_solver_script ?cc health (str-cat "predicate BP(real systolic_blood_pressure, real diastolic_blood_pressure, real pulse_rate) {} fact bp = new BP(systolic_blood_pressure: " (float ?sbd) ", diastolic_blood_pressure: " (float ?dbp) ", pulse_rate: " (float ?pr) ");"))
)

(defrule start_health
    (solver (solver_type health) (solver_ptr ?h) (state idle))
    =>
    (start_execution ?h)
)

(defrule delete_health
    (solver (solver_type health) (solver_ptr ?h) (state finished))
    ?bp <- (blood_pressure)
    =>
    (delete_solver ?h)
    (retract ?bp)
)
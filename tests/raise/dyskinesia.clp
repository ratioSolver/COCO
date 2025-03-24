(defrule dyskinesia
    (User_parkinson (item_id ?user) (parkinson ?parkinson))

    (User_has_ANXIETY (item_id ?user) (ANXIETY ?ANXIETY))
=>
    (bind ?dyskinesia 0)
    (bind ?dyskinesia_relevant (create$))

    (if (and ?parkinson ?ANXIETY) then (bind ?dyskinesia (+ ?dyskinesia 1)))

    (if (and (>= ?dyskinesia 0) (<= ?dyskinesia 1)) then (add_data ?user (create$ physical_distress dyskinesia_relevant) (create$ "Low" ?dyskinesia_relevant)))
    (if (and (>= ?dyskinesia 2) (<= ?dyskinesia 3)) then (add_data ?user (create$ physical_distress dyskinesia_relevant) (create$ "Medium" ?dyskinesia_relevant)))
    (if (>= ?dyskinesia 4) then (add_data ?user (create$ physical_distress dyskinesia_relevant) (create$ "High" ?dyskinesia_relevant)))
)
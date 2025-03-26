(defrule dyskinesia
    (User_parkinson (item_id ?user) (parkinson ?parkinson))

    (User_has_ANXIETY (item_id ?user) (ANXIETY ?ANXIETY))
=>
    (bind ?dyskinesia 0)

    (if (and ?parkinson ?ANXIETY) then (bind ?dyskinesia (+ ?dyskinesia 1)))

    (printout t "User: " ?user crlf)
    (printout t "Dyskinesia: " ?dyskinesia crlf)

    (if (and (>= ?dyskinesia 0) (<= ?dyskinesia 1)) then (add_data ?user (create$ DYSKINESIA) (create$ low)))
    (if (and (>= ?dyskinesia 2) (<= ?dyskinesia 3)) then (add_data ?user (create$ DYSKINESIA) (create$ medium)))
    (if (>= ?dyskinesia 4) then (add_data ?user (create$ DYSKINESIA) (create$ high)))
)
(defrule fluctuation
    (User_parkinson (item_id ?user) (parkinson ?parkinson))

    (User_has_ANXIETY (item_id ?user) (ANXIETY ?ANXIETY))
=>
    (bind ?fluctuation 0)
    (bind ?fluctuation_relevant (create$))

    (if (and ?parkinson ?ANXIETY) then (bind ?fluctuation (+ ?fluctuation 1)))

    (if (and (>= ?fluctuation 0) (<= ?fluctuation 1)) then (add_data ?user (create$ physical_distress fluctuation_relevant) (create$ "Low" ?fluctuation_relevant)))
    (if (and (>= ?fluctuation 2) (<= ?fluctuation 3)) then (add_data ?user (create$ physical_distress fluctuation_relevant) (create$ "Medium" ?fluctuation_relevant)))
    (if (>= ?fluctuation 4) then (add_data ?user (create$ physical_distress fluctuation_relevant) (create$ "High" ?fluctuation_relevant)))
)
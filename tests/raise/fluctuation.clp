(defrule fluctuation
    (User_parkinson (item_id ?user) (parkinson ?parkinson))

    (User_has_ANXIETY (item_id ?user) (ANXIETY ?ANXIETY))
=>
    (bind ?fluctuation 0)

    (if (and ?parkinson ?ANXIETY) then (bind ?fluctuation (+ ?fluctuation 1)))

    (if (and (>= ?fluctuation 0) (<= ?fluctuation 1)) then (add_data ?user (create$ FLUCTUATION) (create$ "Low")))
    (if (and (>= ?fluctuation 2) (<= ?fluctuation 3)) then (add_data ?user (create$ FLUCTUATION) (create$ "Medium")))
    (if (>= ?fluctuation 4) then (add_data ?user (create$ FLUCTUATION) (create$ "High")))
)
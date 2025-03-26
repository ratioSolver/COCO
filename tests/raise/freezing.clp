(defrule freezing
    (User_parkinson (item_id ?user) (parkinson ?parkinson))
    (User_older_adults (item_id ?user) (older_adults ?older_adults))
    (User_psychiatric_patients (item_id ?user) (psychiatric_patients ?psychiatric_patients))
    (User_multiple_sclerosis (item_id ?user) (multiple_sclerosis ?multiple_sclerosis))
    (User_young_pci_autism (item_id ?user) (young_pci_autism ?young_pci_autism))

    (User_has_ANXIETY (item_id ?user) (ANXIETY ?ANXIETY))
    (User_has_crowding (item_id ?user) (crowding ?crowding))
    (User_has_architectural_barriers (item_id ?user) (architectural_barriers ?architectural_barriers))
    (User_has_heart_rate_differential (item_id ?user) (heart_rate_differential ?heart_rate_differential))
    (User_has_lighting (item_id ?user) (lighting ?lighting))
=>
    (bind ?freezing 0)
    (bind ?freezing_relevant (create$))

    (if (and (or ?parkinson ?older_adults ?psychiatric_patients ?multiple_sclerosis ?young_pci_autism) (>= ?crowding 2)) then (bind ?freezing (+ ?freezing 1)))
    (if (and ?parkinson ?ANXIETY) then (bind ?freezing (+ ?freezing 1)))
    (if (and ?parkinson ?architectural_barriers) then (bind ?freezing (+ ?freezing 1)))
    (if (and (or ?parkinson ?young_pci_autism) ?lighting) then (bind ?freezing (+ ?freezing 1)))

    (if (and (or ?psychiatric_patients ?young_pci_autism ?parkinson ?multiple_sclerosis) (>= ?heart_rate_differential 50)) then (bind ?freezing_relevant (insert$ ?freezing_relevant 1 heart_rate_differential)))

    (printout t "User: " ?user crlf)
    (printout t "Freezing: " ?freezing crlf)
    (printout t "Freezing Relevant Factors: " ?freezing_relevant crlf)

    (if (and (>= ?freezing 0) (<= ?freezing 1)) then (add_data ?user (create$ FREEZING freezing_relevant) (create$ low (to_json ?freezing_relevant))))
    (if (and (>= ?freezing 2) (<= ?freezing 3)) then (add_data ?user (create$ FREEZING freezing_relevant) (create$ medium (to_json ?freezing_relevant))))
    (if (>= ?freezing 4) then (add_data ?user (create$ FREEZING freezing_relevant) (create$ high (to_json ?freezing_relevant))))
)
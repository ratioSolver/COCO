(defrule robot_training
  (Robot_has_command_completed (item_id ?robot) (command_completed rot))
  (Robot_has_associated_user (item_id ?robot) (associated_user ?user))  
=>
  (add_data ?robot (create$ current_command) (create$ training))
  (do-for-all-facts ((?domain-fact CognitiveDomain_name)) TRUE
    (printout t "Cognitive Domain: " ?domain-fact:name crlf)
    (if (not (any-factp ((?ex-user-fact ExerciseDone_user) (?ex-type-fact ExerciseDone_exercise_type) (?ex-function-fact ExerciseType_function)) (and (eq ?ex-user-fact:user ?user) (eq ?ex-type-fact:exercise_type ?ex-function-fact:item_id))))
      then
        (printout t "No exercise in domain " ?domain-fact:name " for user " ?user crlf)
        (do-for-fact ((?ex-tp-name-fact ExerciseType_name) (?ex-tp-function-fact ExerciseType_function)) (and (eq ?ex-tp-name-fact:item_id ?ex-tp-function-fact:item_id) (eq ?ex-tp-function-fact:function ?domain-fact:item_id))
          (printout t "Exercise: " ?ex-tp-name-fact:name " is available for user " ?user crlf)
        )
      else
        (printout t "Exercises found in domain " ?domain-fact:name " for user " ?user crlf)      
    )
    (do-for-all-facts ((?ex-tp-name-fact ExerciseType_name) (?ex-tp-function-fact ExerciseType_function)) (and (eq ?ex-tp-name-fact:item_id ?ex-tp-function-fact:item_id) (eq ?ex-tp-function-fact:function ?domain-fact:item_id))
      (printout t "Exercise: " ?ex-tp-name-fact:name crlf)
    )
  )
)
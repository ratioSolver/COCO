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
          (printout t "Exercise: " ?ex-tp-name-fact:name " is available in domain " ?domain-fact:name crlf)
          ; iterate over all the tests done by the user in the given domain and get the lowest performance
          (bind ?min-performance 4)  ; Initialize with high value
          (do-for-all-facts ((?test-done-fact TestDone_user) (?test-done-type-fact TestDone_test_type) (?test-done-performance-fact TestDone_performance)) 
            (and (eq ?test-done-fact:user ?user) (eq ?test-done-type-fact:test_type ?ex-tp-function-fact:item_id))
            (if (< ?test-done-performance-fact:performance ?min-performance)
              then
                (bind ?min-performance ?test-done-performance-fact:performance)
            )
          )
        )
      else
        (printout t "Exercises found in domain " ?domain-fact:name " for user " ?user crlf)      
    )
    (do-for-all-facts ((?ex-tp-name-fact ExerciseType_name) (?ex-tp-function-fact ExerciseType_function)) (and (eq ?ex-tp-name-fact:item_id ?ex-tp-function-fact:item_id) (eq ?ex-tp-function-fact:function ?domain-fact:item_id))
      (printout t "Exercise: " ?ex-tp-name-fact:name crlf)
    )
  )
)
(defrule robot_training
  (Robot_has_command_completed (item_id ?robot) (command_completed rot))
  (Robot_has_associated_user (item_id ?robot) (associated_user ?user))  
=>
  (add_data ?robot (create$ current_command) (create$ training))
  (do-for-all-facts ((?domain CognitiveDomain_name)) TRUE
    (printout t "Cognitive Domain: " ?domain:name crlf)
    ; Check if there are any exercises done by the user in the current domain
    (if (not (any-factp ((?ex-done-user ExerciseDone_user) (?ex-done-type ExerciseDone_exercise_type) (?ex-type-function ExerciseType_function)) (and (eq ?ex-done-user:item_id ?ex-done-type:item_id) (eq ?ex-done-type:exercise_type ?ex-type-function:item_id) (eq ?ex-done-user:user ?user) (eq ?ex-type-function:function ?domain:item_id))))
      then
        (printout t "User '" ?user "' has not done any exercises in domain '" ?domain:name "' yet" crlf)
        ; Get an exercise type that is available in the domain
        (do-for-fact ((?ex-type-name ExerciseType_name) (?ex-type-function ExerciseType_function)) (and (eq ?ex-type-name:item_id ?ex-type-function:item_id) (eq ?ex-type-function:function ?domain:item_id))
          (printout t "Exercise '" ?ex-type-name:name "' is available in domain '" ?domain:name "'" crlf)
          ; iterate over all the tests done by the user in the given domain and get the lowest performance
          (bind ?min-performance 4)  ; Initialize with high value
          (do-for-all-facts ((?test-done-user TestDone_user) (?test-done-type TestDone_test_type) (?test-done-performance TestDone_performance) (?cog-test-function CognitiveTest_function)) 
            (and (eq ?test-done-user:item_id ?test-done-type:item_id)
                 (eq ?test-done-user:item_id ?test-done-performance:item_id)
                 (eq ?test-done-user:user ?user)
                 (eq ?test-done-type:test_type ?cog-test-function:item_id)
                 (eq ?cog-test-function:function ?domain:item_id))
            (printout t "Test " ?test-done-type:item_id " done by user " ?user " with performance " ?test-done-performance:performance crlf)
            (if (< ?test-done-performance:performance ?min-performance)
              then
                (bind ?min-performance ?test-done-performance:performance)
            )
          )
          (printout t "User's performance in " ?domain:name " is: " ?min-performance crlf)
          (if (eq ?min-performance 0)
            then
              (enqueue-exercise ?ex-type-name:item_id 1)
              (enqueue-exercise ?ex-type-name:item_id 1)
          )
          (if (eq ?min-performance 1)
            then
              (enqueue-exercise ?ex-type-name:item_id 2)
              (enqueue-exercise ?ex-type-name:item_id 2)
          )
          (if (> ?min-performance 1)
            then
              (enqueue-exercise ?ex-type-name:item_id 3)
              (enqueue-exercise ?ex-type-name:item_id 3)
              (enqueue-exercise ?ex-type-name:item_id 3)
              (enqueue-exercise ?ex-type-name:item_id 3)
          )        
        )
      else
        (printout t "Exercises found in domain " ?domain:name " for user " ?user crlf)      
    )
  )
  (do-for-fact ((?ex exercise)) (not (any-factp ((?ex2 exercise)) (< ?ex2:id ?ex:id)))
    (printout t "Executing exercise: " ?ex:exercise-type " - " ?ex:exercise-level crlf)
    (add_data ?robot (create$ current_exercise current_exercise_level) (create$ ?ex:exercise-type ?ex:exercise-level))
    (retract ?ex)
  )
)

(defrule exercise_done
  (Robot_has_current_exercise (item_id ?robot) (current_exercise ?exercise))
  (Robot_has_current_exercise_level (item_id ?robot) (current_exercise_level ?level))
  (Robot_has_current_exercise_performance (item_id ?robot) (current_exercise_performance ?performance))
  (Robot_has_associated_user (item_id ?robot) (associated_user ?user))
=>
  (printout t "Exercise " ?exercise " at level " ?level " completed with performance " ?performance " by user " ?user crlf)
  (if (any-factp ((?ex-done ExerciseDone_user) (?ex-done-exercise ExerciseDone_exercise_type)) 
      (and (eq ?ex-done:item_id ?ex-done-exercise:item_id) (eq ?ex-done:user ?user) (eq ?ex-done-exercise:item_id ?exercise)))
    then
      (set_properties ?ex-done:item_id (create$ level performance) (create$ ?level ?performance))
    else
      (assert (ExerciseDone (user ?user) (exercise_type ?exercise) (performance ?performance)))
  )
)
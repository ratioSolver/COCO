(defrule dry_garden
    (Garden_has_humidity (item_id ?garden) (humidity ?humidity&:(< ?humidity 400)))
    (Garden_sprinkler (item_id ?garden) (sprinkler ?sprinkler))
=>
    (if (not (any-factp ((?f executor)) (eq ?f:name ?sprinkler)))
        then
            (bind ?problem (str-cat "class Sprinkler : StateVariable { predicate State(int humidity) { duration >= 1.0; } predicate Irrigate() { duration >= 1.0; goal s = new State(end:start); } predicate WellBeing() { duration >= 1.0; goal i = new Irrigate(end:start);}};"))
            (bind ?problem (str-cat ?problem " Sprinkler s = new Sprinkler(); fact f0 = new s.State(humidity:" ?humidity ",start:origin); goal g0 = new s.WellBeing(end:horizon);"))
            (create_executor_script ?sprinkler ?problem)
    )
)
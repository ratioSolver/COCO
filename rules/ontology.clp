(deftemplate is_subtype_of (slot parent_id (type SYMBOL)) (slot child_id (type SYMBOL)))
(deftemplate is_disjoint_with (slot type_id1 (type SYMBOL)) (slot type_id2 (type SYMBOL)))
(deftemplate has_property (slot type_id (type SYMBOL)) (slot domain_id (type SYMBOL)) (slot name (type STRING)) (slot min (type INTEGER)) (slot max (type INTEGER)) (slot transitive (type SYMBOL) (default FALSE)))

(defrule new_subtype
    (is_subtype_of (parent_id ?parent_id) (child_id ?child_id))
    (is_instance_of (item_id ?child) (type_id ?child_type))
    (not (is_instance_of (item_id ?parent) (type_id ?parent_type)))
    =>
    (assert (is_instance_of (item_id ?parent) (type_id ?child_type)))
)

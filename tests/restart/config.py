import sys
import requests
import logging

logger = logging.getLogger(__name__)
logger.setLevel(logging.DEBUG)

handler = logging.StreamHandler(sys.stdout)
handler.setLevel(logging.DEBUG)
logger.addHandler(handler)


def create_types(session: requests.Session, url: str):
    # Create the CognitiveDomain type..
    response = session.post(url + '/types', json={
        'name': 'CognitiveDomain',
        'static_properties': {
            'name': {'type': 'string'}
        }
    })
    if response.status_code != 204:
        logger.error('Failed to create type ExecutiveFunction')
        return

    # Create the CognitiveTest type..
    response = session.post(url + '/types', json={
        'name': 'CognitiveTest',
        'static_properties': {
            'name': {'type': 'string'},
            # Funzione cognitiva associata
            'function': {'type': 'item', 'domain': 'CognitiveDomain'}
        }
    })
    if response.status_code != 204:
        logger.error('Failed to create type CognitiveTest')
        return

    # Create the User type..
    response = session.post(url + '/types', json={
        'name': 'User',
        'static_properties': {
            'name': {'type': 'string'}
        }
    })
    if response.status_code != 204:
        logger.error('Failed to create type User')
        return

    # Create the TestDone type..
    response = session.post(url + '/types', json={
        'name': 'TestDone',
        'static_properties': {
            # Test svolto
            'test_type': {'type': 'item', 'domain': 'CognitiveTest'},
            # Utente che ha svolto il test
            'user': {'type': 'item', 'domain': 'User'},
            # Performance del test (0-4)
            'performance': {'type': 'int', 'min': 0, 'max': 4}
        }
    })
    if response.status_code != 204:
        logger.error('Failed to create type TestDone')
        return

    # Create the ExerciseType type..
    response = session.post(url + '/types', json={
        'name': 'ExerciseType',
        'static_properties': {
            'name': {'type': 'string'},
            # Durata in minuti
            'duration': {'type': 'int', 'min': 0, 'max': 60},
            # Funzione cognitiva associata
            'function': {'type': 'item', 'domain': 'CognitiveDomain'}
        }
    })
    if response.status_code != 204:
        logger.error('Failed to create type ExerciseType')
        return

    # Create the ExerciseDone type..
    response = session.post(url + '/types', json={
        'name': 'ExerciseDone',
        'static_properties': {
            # Esercizio svolto
            'exercise_type': {'type': 'item', 'domain': 'ExerciseType'},
            # Utente che ha svolto l'esercizio
            'user': {'type': 'item', 'domain': 'User'},
            # Livello di difficoltà dell'esercizio (0-6)
            'difficulty': {'type': 'int', 'min': 0, 'max': 6},
            # Performance dell'esercizio (0-1)
            'performance': {'type': 'float', 'min': 0, 'max': 1}
        }
    })
    if response.status_code != 204:
        logger.error('Failed to create type ExerciseDone')
        return

    # Create the Robot type..
    response = session.post(url + '/types', json={
        'name': 'Robot',
        'static_properties': {
            'name': {'type': 'string'}  # Nome del robot
        },
        'dynamic_properties': {
            'current_command': {'type': 'symbol', 'values': ['welcome', 'rot', 'training', 'goodbye']},
            'current_modality': {'type': 'symbol', 'values': ['formal', 'informal']},
            'command_completed': {'type': 'symbol', 'values': ['welcome', 'rot', 'training', 'goodbye']},
            'associated_user': {'type': 'item', 'domain': 'User'},
            'current_exercise': {'type': 'item', 'domain': 'ExerciseType'},
            'current_exercise_level': {'type': 'int', 'min': 0, 'max': 6},
            'current_exercise_performance': {'type': 'float', 'min': 0, 'max': 1}
        }
    })
    if response.status_code != 204:
        logger.error('Failed to create type Robot')
        return


def create_rules(session: requests.Session, url: str):
    # Create the next-id rule
    response = session.post(url + '/reactive_rules', json={
        'name': 'next_id',
        'content': '(defglobal ?*next-id* = 1)'
    })
    if response.status_code != 204:
        logger.error('Failed to create rule next_id')
        return

    # Create the exercise template
    response = session.post(url + '/reactive_rules', json={
        'name': 'exercise_template',
        'content': '(deftemplate exercise (slot id) (slot exercise-type (type SYMBOL)) (slot exercise-level (type INTEGER) (range 0 6)))'
    })
    if response.status_code != 204:
        logger.error('Failed to create rule exercise_template')
        return

    # Create the enqueue-exercise function
    response = session.post(url + '/reactive_rules', json={
        'name': 'enqueue_exercise',
        'content': '(deffunction enqueue-exercise (?type ?level) (bind ?id ?*next-id*) (bind ?*next-id* (+ ?*next-id* 1)) (assert (exercise (id ?id) (exercise-type ?type) (exercise-level ?level))))'
    })
    if response.status_code != 204:
        logger.error('Failed to create rule enqueue_exercise')
        return

    # Associating a user to a robot starts a session by welcoming the user
    response = session.post(url + '/reactive_rules', json={
        'name': 'robot_session', 'content': '(defrule robot_session (Robot_has_associated_user (item_id ?robot) (associated_user ?user)) => (add_data ?robot (create$ current_command current_modality) (create$ welcome formal)))'})
    if response.status_code != 204:
        logger.error('Failed to create rule robot_session')
        return

    # Completing the welcome command triggers the robot to switch to the rot command
    response = session.post(url + '/reactive_rules', json={
        'name': 'robot_rot', 'content': '(defrule robot_rot (Robot_has_command_completed (item_id ?robot) (command_completed welcome)) => (add_data ?robot (create$ current_command) (create$ rot)))'})
    if response.status_code != 204:
        logger.error('Failed to create rule robot_rot')
        return

    # Completing the rot command triggers the robot to switch to the training command
    with open('robot_training.clp', 'r') as file:
        robot_training = file.read()
    response = session.post(
        url + '/reactive_rules', json={'name': 'robot_training', 'content': robot_training})
    if response.status_code != 204:
        logger.error('Failed to create rule robot_training')
        return

    # Completing the training command triggers the robot to switch to the goodbye command
    response = session.post(url + '/reactive_rules', json={
        'name': 'robot_goodbye', 'content': '(defrule robot_goodbye (Robot_has_command_completed (item_id ?robot) (command_completed training)) => (add_data ?robot (create$ current_command) (create$ goodbye)))'})
    if response.status_code != 204:
        logger.error('Failed to create rule robot_goodbye')
        return

    # Completing the goodbye command ends the session
    response = session.post(url + '/reactive_rules', json={
        'name': 'robot_end_session', 'content': '(defrule robot_end_session (Robot_has_command_completed (item_id ?robot) (command_completed goodbye)) => (add_data ?robot (create$ associated_user command_completed current_command) (create$ nil nil nil nil)))'})
    if response.status_code != 204:
        logger.error('Failed to create rule robot_end_session')
        return


def create_items(session: requests.Session, url: str) -> list[str]:
    # Create cognitive domain items
    cognitive_domains = ['Memory', 'Attention', 'ExecutiveFunction', 'General']
    cognitive_domain_ids = []
    for domain in cognitive_domains:
        response = session.post(
            url + '/items', json={'type': 'CognitiveDomain', 'properties': {'name': domain}})
        if response.status_code != 201:
            logger.error(f'Failed to create CognitiveDomain item for {domain}')
            return
        cognitive_domain_ids.append(response.text)

    # Create cognitive test items
    cognitive_tests = [
        {'name': 'MoCa',
            'function': cognitive_domain_ids[3]},
        {'name': 'Matrici_Attentive',
            'function': cognitive_domain_ids[1]},
        {'name': 'Trial_Making_Test_A',
            'function': cognitive_domain_ids[2]},
        {'name': 'Trial_Making_Test_B',
            'function': cognitive_domain_ids[2]},
        {'name': 'Trial_Making_Test_B_A',
            'function': cognitive_domain_ids[2]},
        {'name': 'Fluenza_semantica',
            'function': cognitive_domain_ids[2]},
        {'name': 'Fluenza_fonologica',
            'function': cognitive_domain_ids[2]},
        {'name': 'Modified_Winsconsin_Card_Sorting_Test',
            'function': cognitive_domain_ids[2]},
        {'name': 'Breve_racconto', 'function': cognitive_domain_ids[0]}
    ]
    cognitive_test_ids = []
    for test in cognitive_tests:
        response = session.post(
            url + '/items', json={'type': 'CognitiveTest', 'properties': test})
        if response.status_code != 201:
            logger.error(
                f'Failed to create CognitiveTest item for {test["name"]}')
            return
        cognitive_test_ids.append(response.text)

    # Create exercise type items
    exercise_types = [
        {'name': 'memoria visiva', 'duration': 5,
            'function': cognitive_domain_ids[0]},
        {'name': 'Att_Attenzione divisa', 'duration': 5,
            'function': cognitive_domain_ids[1]},
        {'name': 'fluenza verbale', 'duration': 5,
            'function': cognitive_domain_ids[2]}
    ]
    exercise_type_ids = []
    for exercise in exercise_types:
        response = session.post(
            url + '/items', json={'type': 'ExerciseType', 'properties': exercise})
        if response.status_code != 201:
            logger.error(
                f'Failed to create ExerciseType item for {exercise["name"]}')
            return
        exercise_type_ids.append(response.text)

    # Create user items
    response = session.post(
        url + '/items', json={'type': 'User', 'properties': {'name': 'User1'}})
    if response.status_code != 201:
        logger.error('Failed to create User item')
        return
    user_id = response.text
    # Create test done items
    test_done_items = [
        {'test_type': cognitive_test_ids[0],
            'user': user_id, 'performance': 3},
        {'test_type': cognitive_test_ids[1],
            'user': user_id, 'performance': 2},
        {'test_type': cognitive_test_ids[2],
            'user': user_id, 'performance': 4},
        {'test_type': cognitive_test_ids[3],
            'user': user_id, 'performance': 1},
        {'test_type': cognitive_test_ids[4],
            'user': user_id, 'performance': 3},
        {'test_type': cognitive_test_ids[5],
            'user': user_id, 'performance': 2},
        {'test_type': cognitive_test_ids[6],
            'user': user_id, 'performance': 4},
        {'test_type': cognitive_test_ids[7],
            'user': user_id, 'performance': 1}
    ]
    for test in test_done_items:
        response = session.post(
            url + '/items', json={'type': 'TestDone', 'properties': test})
        if response.status_code != 201:
            logger.error(
                f'Failed to create TestDone item for {test["test_type"]}')
            return

    # Create robot item
    response = session.post(
        url + '/items', json={'type': 'Robot', 'properties': {'name': 'Robot'}})
    if response.status_code != 201:
        logger.error('Failed to create Robot item')
        return


if __name__ == '__main__':
    url = sys.argv[1] if len(sys.argv) > 1 else 'http://localhost:8080'
    session = requests.Session()

    create_types(session, url)
    create_rules(session, url)
    create_items(session, url)

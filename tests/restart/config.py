import sys
import requests
import logging

logger = logging.getLogger(__name__)
logger.setLevel(logging.DEBUG)

handler = logging.StreamHandler(sys.stdout)
handler.setLevel(logging.DEBUG)
logger.addHandler(handler)


def create_types(session: requests.Session, url: str):
    # Create the user type..
    response = session.post(url + '/types', json={
        'name': 'User',
        'static_properties': {
            'name': {'type': 'string'},
            'MoCa': {'type': 'int', 'min': 0, 'max': 4},
            'Matrici_Attentive': {'type': 'int', 'min': 0, 'max': 4},
            'Trial_Making_Test_A': {'type': 'int', 'min': 0, 'max': 4},
            'Trial_Making_Test_B': {'type': 'int', 'min': 0, 'max': 4},
            'Trial_Making_Test_B_A': {'type': 'int', 'min': 0, 'max': 4},
            'Fluenza_semantica': {'type': 'int', 'min': 0, 'max': 4},
            'Fluenza_fonologica': {'type': 'int', 'min': 0, 'max': 4},
            'Modified_Winsconsin_Card_Sorting_Test': {'type': 'int', 'min': 0, 'max': 4},
            'Breve_racconto': {'type': 'int', 'min': 0, 'max': 4}
        }
    })
    if response.status_code != 204:
        logger.error('Failed to create type User')
        return

    # Create the Robot type..
    response = session.post(url + '/types', json={
        'name': 'Robot',
        'static_properties': {
            'name': {'type': 'string'}  # Nome del robot
        },
        'dynamic_properties': {
            # Allora consideriamo (welcome, rot, training, goodbye)
            'current_command': {'type': 'symbol', 'values': ['welcome', 'rot', 'training', 'goodbye']},
            # Modalità (formale/informale)
            'current_modality': {'type': 'symbol', 'values': ['formal', 'informal']},
            # Comando completato (associati a idle (session_start) a rot(welcome_done") a goodbye(end_session))
            'command_completed': {'type': 'symbol', 'values': ['welcome', 'rot', 'training', 'goodbye']},
            'associated_user': {'type': 'item', 'domain': 'User'}
        }
    })
    if response.status_code != 204:
        logger.error('Failed to create type Robot')
        return

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
            # Performance dell'esercizio (0-1)
            'performance': {'type': 'float', 'min': 0, 'max': 1}
        }
    })
    if response.status_code != 204:
        logger.error('Failed to create type ExerciseDone')
        return


def create_rules(session: requests.Session, url: str):
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
    response = session.post(url + '/reactive_rules', json={
        'name': 'robot_training', 'content': '(defrule robot_training (Robot_has_command_completed (item_id ?robot) (command_completed rot)) => (add_data ?robot (create$ current_command) (create$ training)))'})
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
    # Create robot item
    response = session.post(
        url + '/items', json={'type': 'Robot', 'properties': {'name': 'Robot'}})
    if response.status_code != 201:
        logger.error('Failed to create Robot item')
        return

    # Create cognitive domain items
    cognitive_domains = ['Memory', 'Attention', 'ExecutiveFunction']
    cognitive_domain_ids = []
    for domain in cognitive_domains:
        response = session.post(
            url + '/items', json={'type': 'CognitiveDomain', 'properties': {'name': domain}})
        if response.status_code != 201:
            logger.error(f'Failed to create CognitiveDomain item for {domain}')
            return
        cognitive_domain_ids.append(response.text)

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
        url + '/items', json={'type': 'User', 'properties': {'name': 'User1', 'MoCa': 1, 'Matrici_Attentive': 2, 'Trial_Making_Test_A': 3, 'Trial_Making_Test_B': 4, 'Trial_Making_Test_B_A': 1, 'Fluenza_semantica': 2, 'Fluenza_fonologica': 3, 'Modified_Winsconsin_Card_Sorting_Test': 4, 'Breve_racconto': 1}})
    if response.status_code != 201:
        logger.error('Failed to create User item')
        return


if __name__ == '__main__':
    url = sys.argv[1] if len(sys.argv) > 1 else 'http://localhost:8080'
    session = requests.Session()

    create_types(session, url)
    create_rules(session, url)
    create_items(session, url)

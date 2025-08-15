import sys
import requests
import logging

logger = logging.getLogger(__name__)
logger.setLevel(logging.DEBUG)

handler = logging.StreamHandler(sys.stdout)
handler.setLevel(logging.DEBUG)
logger.addHandler(handler)


def create_types(session, url):
    types = [
        {'name': 'User', 'static_properties': { 'name': {'type': 'string'}, 'age': {'type': 'int'}}},
        {'name': 'Robot', 'static_properties': { 'name': {'type': 'string'}},
            'dynamic_properties': {
                'saying': {'type': 'string'},
                'understood': {'type': 'string'},
                'face': {'type': 'symbol', 'values': ['happy_talking.gif', 'happy.gif', 'idle.gif', 'laugh.gif', 'listening.gif', 'sad_talking.gif', 'sad.gif', 'talking.gif']},
                'user': {'type': 'item', 'domain': 'User'},
                'listening': {'type': 'bool'}
            }
        }
    ]
    for type in types:
        response = session.post(url + '/types', json=type)
        if response.status_code != 204:
            logger.error(f'Failed to create type {type["name"]}')
            return
        logger.info(f'Type {type["name"]} created successfully')


def create_intents(session, url):
    intents = [
        {'name': 'affirmative', 'description': 'The user agrees with the system'},
        {'name': 'negative', 'description': 'The user disagrees with the system'},
        {'name': 'greet', 'description': 'The user greets the system'},
        {'name': 'farewell', 'description': 'The user says goodbye to the system'},
        {'name': 'presentation', 'description': 'The user introduces himself/herself to the system'},
        {'name': 'ask_something', 'description': 'The user asks something to the system'}
    ]
    for intent in intents:
        response = session.post(url + '/intents', json=intent)
        if response.status_code != 204:
            logger.error(f'Failed to create intent {intent["name"]}')
            return
        logger.info(f'Intent {intent["name"]} created successfully')


def create_entities(session, url):
    entities = [
        {'name': 'user_name', 'type': 'string', 'description': "The name of the user. Don't set this entity if the user is not saying his/her name"},
        {'name': 'user_age', 'type': 'int', 'description': "The age of the user. Don't set this entity if the user is not saying his/her age"},
        {'name': 'robot_response', 'type': 'string', 'description': "Always set this entity with a message in italian answering, commenting or requesting for clarification to the user's input, in the same tone used by the user. Try to be creative and funny."},
        {'name': 'robot_face', 'type': 'symbol', 'description': "Always set this entity with the face of the robot according to the robot's message. Set this entity to a string among: 'happy_talking.gif', 'happy.gif', 'idle.gif', 'laugh.gif', 'listening.gif', 'sad_talking.gif', 'sad.gif', 'talking.gif'. This entity is used to set the face of the robot."},
        {'name': 'robot_ask', 'type': 'bool', 'description': "Always set this entity to true if the robot is asking a question. Set this entity to false if the robot is not asking a question."}
    ]
    for entity in entities:
        response = session.post(url + '/entities', json=entity)
        if response.status_code != 204:
            logger.error(f'Failed to create entity {entity["name"]}')
            return
        logger.info(f'Entity {entity["name"]} created successfully')


def create_slots(session, url):
    slots = [
        {'name': 'user_name', 'type': 'string', 'description': "The name of the user"},
        {'name': 'user_age', 'type': 'int', 'description': "The age of the user"}
    ]
    for slot in slots:
        response = session.post(url + '/slots', json=slot)
        if response.status_code != 204:
            logger.error(f'Failed to create slot {slot["name"]}')
            return
        logger.info(f'Slot {slot["name"]} created successfully')


def create_rules(session, url):
    rules = [
        { # This rule is used to understand, through a LLM, what the robot has understood from the user's input.
            'name': 'understand',
            'content': '(defrule understand (Robot_has_understood (item_id ?robot) (understood ?understood)) => (understand ?robot ?understood) (add_data ?robot (create$ understood) (create$ nil)))'
        },
        { # This rule is used to set the age of the user.
            'name': 'set_age',
            'content': '(defrule set_age ?f <- (entity (item_id ?robot) (name user_age) (value ?value)) (Robot_has_user (item_id ?robot) (user ?user)) => (set_slots ?robot (create$ user_age) (create$ ?value)) (set_properties ?user (create$ age) (create$ ?value)) (retract ?f))'
        },
        { # This rule is used to set the name of the user.
            'name': 'set_name',
            'content': '(defrule set_name ?f <- (entity (item_id ?robot) (name user_name) (value ?value)) (Robot_has_user (item_id ?robot) (user ?user)) => (set_slots ?robot (create$ user_name) (create$ ?value)) (set_properties ?user (create$ name) (create$ ?value)) (retract ?f))'
        },
        { # This rule is used to set the response of the robot.
            'name': 'set_response',
            'content': '(defrule set_response ?f <- (entity (item_id ?robot) (name robot_response) (value ?value)) => (add_data ?robot (create$ saying) (create$ ?value)) (retract ?f))'
        },
        { # This rule is used to set the face of the robot.
            'name': 'set_face',
            'content': '(defrule set_face ?f <- (entity (item_id ?robot) (name robot_face) (value ?value)) => (add_data ?robot (create$ face) (create$ ?value)) (retract ?f))'
        },
        { # This rule is used to set the listening state of the robot.
            'name': 'set_robot_listening',
            'content': '(defrule set_robot_listening ?f <- (entity (item_id ?robot) (name robot_ask) (value ?value)) => (add_data ?robot (create$ listening) (create$ ?value)) (retract ?f))'
        }
    ]
    for rule in rules:
        response = session.post(url + '/reactive_rules', json=rule)
        if response.status_code != 204:
            logger.error(f'Failed to create rule {rule["name"]}')
            return
        logger.info(f'Rule {rule["name"]} created successfully')


def create_items(session, url):
    response = session.post(url + '/items', json={'type': 'User'})
    if response.status_code != 201:
        logger.error('Failed to create User item')
        return
    user_id = response.text
    logger.info('User item created successfully with ID: ' + user_id)

    response = session.post(url + '/items', json={'type': 'Robot', 'properties': {'name': 'Robot'}})
    if response.status_code != 201:
        logger.error('Failed to create Robot item')
        return
    robot_id = response.text
    logger.info('Robot item created successfully with ID: ' + robot_id)

    response = session.post(url + '/data/' + robot_id, json={
        'face': 'idle.gif',
        'user': user_id
    })
    if response.status_code != 204:
        logger.error('Failed to set initial data for Robot item')
        return


if __name__ == '__main__':
    url = sys.argv[1] if len(sys.argv) > 1 else 'https://192.168.100.17:8080'
    session = requests.Session()
    session.verify = 'cert.pem'  # Use your actual cert path

    create_types(session, url)
    create_intents(session, url)
    create_entities(session, url)
    create_slots(session, url)
    create_rules(session, url)
    create_items(session, url)
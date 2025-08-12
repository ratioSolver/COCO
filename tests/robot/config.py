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
        {'name': 'Robot', 'static_properties': {
            'saying': {'type': 'string'},
            'understood': {'type': 'string'},
            'robot_face': {'type': 'symbol', 'values': ['happy', 'sad', 'angry', 'surprised', 'neutral']},
            'user': {'type': 'item', 'domain': 'User'},
            'listening': {'type': 'bool'}
        }}
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
        {'name': 'robot_face', 'type': 'symbol', 'values': ['happy_talking', 'happy', 'idle', 'laugh', 'listening', 'sad_talking', 'sad', 'talking'], 'description': "Always set this entity with the face of the robot according to the robot's message. Set this entity to a string among: 'happy', 'sad', 'angry', 'surprised', 'neutral'. This entity is used to set the face of the robot."},
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


if __name__ == '__main__':
    url = sys.argv[1] if len(sys.argv) > 1 else 'http://localhost:8080'
    session = requests.Session()

    create_types(session, url)
    create_intents(session, url)
    create_entities(session, url)
    create_slots(session, url)
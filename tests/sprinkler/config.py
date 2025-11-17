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
        {'name': 'Sprinkler',
         'dynamic_properties': {'status': {'type': 'symbol', 'values': ['on', 'off']}}},
        {'name': 'Garden',
         'static_properties': {'sprinkler': {'type': 'item', 'domain': 'Sprinkler'}},
         'dynamic_properties': {'humidity': {'type': 'int', 'min': 0, 'max': 1023}}}
    ]
    for type in types:
        response = session.post(url + '/types', json=type)
        if response.status_code != 204:
            logger.error(f'Failed to create type {type["name"]}')
            return
        logger.info(f'Type {type["name"]} created successfully')


def create_reactive_rules(session, url):
    with open("sprinkler.clp", "r") as f:
        content = f.read()
        response = session.post(url + '/reactive_rules', json={'name': 'dry_garden', 'content': content})
        if response.status_code != 204:
            logger.error('Failed to create reactive rule dry_garden')
            return
        logger.info('Reactive rule dry_garden created successfully')        


def create_items(session, url):
    response = session.post(url + '/items', json={'type': 'Sprinkler'})
    if response.status_code != 201:
        logger.error('Failed to create Sprinkler item')
        return
    sprinkler_id = response.text
    logger.info('Sprinkler item created successfully with ID: ' + sprinkler_id)

    response = session.post(
        url + '/items', json={'type': 'Garden', 'properties': {'sprinkler': sprinkler_id}})
    if response.status_code != 201:
        logger.error('Failed to create Garden item')
        return
    garden_id = response.text
    logger.info('Garden item created successfully with ID: ' + garden_id)


if __name__ == '__main__':
    url = sys.argv[1] if len(sys.argv) > 1 else 'http://localhost:8080'
    session = requests.Session()

    create_types(session, url)
    create_reactive_rules(session, url)
    create_items(session, url)

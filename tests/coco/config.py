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
        {'name': 'User',
         'static_properties': {'name': {'type': 'string'}}}
    ]
    for type in types:
        response = session.post(url + '/types', json=type)
        if response.status_code != 204:
            logger.error(f'Failed to create type {type["name"]}')
            return
        logger.info(f'Type {type["name"]} created successfully')


def create_items(session, url):
    response = session.post(
        url + '/items', json={'type': 'User', 'properties': {'name': 'Alice'}})
    if response.status_code != 201:
        logger.error('Failed to create User item')
        return
    user_id = response.text
    logger.info('User item created successfully with ID: ' + user_id)


if __name__ == '__main__':
    url = sys.argv[1] if len(sys.argv) > 1 else 'http://localhost:8080'
    session = requests.Session()

    create_types(session, url)
    create_items(session, url)

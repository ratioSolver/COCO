import sys
import requests
import logging
from faker import Faker

logger = logging.getLogger(__name__)
logger.setLevel(logging.DEBUG)

handler = logging.StreamHandler(sys.stdout)
handler.setLevel(logging.DEBUG)
logger.addHandler(handler)

fake = Faker('it_IT')


def create_types(session, url):
    # Create the user type..
    response = session.post(url + '/type', json={
        'name': 'User',
        'static_properties': {
            'name': {'type': 'string'},
            'baseline_nutrition': {'type': 'bool'},
            'baseline_fall': {'type': 'int'},
            #    'baseline_rehabilitation_school_load': {'type': 'bool'},
            #    'comorbidities': {'type': 'bool'},
            #    'bipolar_disorder_diagnosis': {'type': 'bool'},
            #    'disability_level': {'type': 'bool'},
            #    'intellectual_disability': {'type': 'bool'},
            #    'traumatic_events': {'type': 'bool'},
            'baseline_freezing': {'type': 'int'},
            'baseline_heart_rate': {'type': 'int'},
            #    'social_judgment': {'type': 'bool'},
            #    'motor_deficit_level': {'type': 'bool'},
            #    'agoraphobia_avoidance_symptoms': {'type': 'bool'},
            #    'panic_attacks_anticipatory_anxiety': {'type': 'bool'},
            #    'somatoform_disorders': {'type': 'bool'},
            #    'social_phobia': {'type': 'bool'},
            'state_anxiety_presence': {'type': 'int'},
            'baseline_blood_pressure': {'type': 'int'},
            'sensory_profile': {'type': 'bool'},
            'stress': {'type': 'int'},
            'psychiatric_disorders': {'type': 'bool'},
            #    'personality_traits': {'type': 'bool'},
            'parkinson': {'type': 'bool'},
            'older_adults': {'type': 'bool'},
            'psychiatric_patients': {'type': 'bool'},
            'multiple_sclerosis': {'type': 'bool'},
            'young_pci_autism': {'type': 'bool'}
        },
        'dynamic_properties': {
            'EXCESSIVE_HEAT': {'type': 'symbol', 'values': ['low', 'medium', 'high']},
            'excessive_heat_relevant': {'type': 'symbol', 'multiple': True, 'values': ['water_balance', 'heart_rate', 'baseline_heart_rate', 'heart_rate_differential', 'galvanic_skin_response', 'baseline_blood_pressure', 'low_blood_pressure', 'sweating', 'body_temperature']},
            'ANXIETY': {'type': 'symbol', 'values': ['low', 'medium', 'high']},
            'anxiety_relevant': {'type': 'symbol', 'multiple': True, 'values': ['baseline_freezing', 'recent_freezing_episodes', 'heart_rate', 'baseline_heart_rate', 'heart_rate_differential', 'respiratory_rate', 'galvanic_skin_response', 'high_blood_pressure', 'self_perception', 'sweating', 'body_temperature']},
            'MENTAL_FATIGUE': {'type': 'symbol', 'values': ['low', 'medium', 'high']},
            'PHYSICAL_FATIGUE': {'type': 'symbol', 'values': ['low', 'medium', 'high']},
            'physical_fatigue_relevant': {'type': 'symbol', 'multiple': True, 'values': ['bar_restaurant', 'water_fountains', 'heart_rate', 'baseline_heart_rate', 'respiratory_rate', 'sittings', 'restroom_availability', 'green_spaces']},
            'SENSORY_DYSREGULATION': {'type': 'symbol', 'values': ['low', 'medium', 'high']},
            'sensory_disregulation_relevant': {'type': 'symbol', 'multiple': True, 'values': ['heart_rate', 'baseline_heart_rate', 'respiratory_rate', 'sensory_profile']},
            'FREEZING': {'type': 'symbol', 'values': ['low', 'medium', 'high']},
            'freezing_relevant': {'type': 'symbol', 'multiple': True, 'values': ['heart_rate_differential']},
            'FLUCTUATION': {'type': 'symbol', 'values': ['low', 'medium', 'high']},
            'DYSKINESIA': {'type': 'symbol', 'values': ['low', 'medium', 'high']},
            'crowding': {'type': 'int'},
            'altered_nutrition': {'type': 'bool'},
            'altered_thirst_perception': {'type': 'int'},
            'bar_restaurant': {'type': 'bool'},
            'architectural_barriers': {'type': 'bool'},
            'water_balance': {'type': 'int'},
            #    'fall': {'type': 'bool'},
            #    'attention_capacity': {'type': 'bool'},
            'sleep_duration_quality': {'type': 'int'},
            #    'engagement_in_adl': {'type': 'bool'},
            'water_fountains': {'type': 'bool'},
            'recent_freezing_episodes': {'type': 'int'},
            'heart_rate': {'type': 'int'},
            'heart_rate_differential': {'type': 'int'},
            'public_events_frequency': {'type': 'bool'},
            'respiratory_rate': {'type': 'int'},
            'galvanic_skin_response': {'type': 'int'},
            'lighting': {'type': 'bool'},
            'noise_pollution': {'type': 'int'},
            'user_reported_noise_pollution': {'type': 'int'},
            'air_pollution': {'type': 'int'},
            'traffic_levels': {'type': 'int'},
            'lack_of_ventilation': {'type': 'int'},
            #    'daily_steps': {'type': 'bool'},
            #    'rehabilitation_school_load': {'type': 'bool'},
            #    'heat_waves': {'type': 'bool'},
            'path_slope': {'type': 'bool'},
            'safety_perception': {'type': 'bool'},
            #    'fatigue_perception': {'type': 'bool'},
            'rough_path': {'type': 'bool'},
            'public_events_presence': {'type': 'bool'},
            'high_blood_pressure': {'type': 'int'},
            'low_blood_pressure': {'type': 'int'},
            'social_pressure': {'type': 'bool'},
            'sittings': {'type': 'bool'},
            'self_perception': {'type': 'bool'},
            'restroom_availability': {'type': 'bool'},
            'sweating': {'type': 'int'},
            'ambient_temperature': {'type': 'int'},
            'body_temperature': {'type': 'int'},
            'ambient_humidity': {'type': 'int'},
            'excessive_urbanization': {'type': 'bool'},
            'green_spaces': {'type': 'bool'}
        }
    })

    if response.status_code != 204:
        logger.error('Failed to create type User')
        return


def create_rules(session, url):
    # Create Anxiety rule..
    with open('anxiety.clp', 'r') as file:
        data = file.read()
    logger.info('Creating the Anxiety rule')
    response = requests.post(
        url + '/reactive_rule', json={'name': 'anxiety', 'content': data})
    if response.status_code != 204:
        logger.error(response.json())
        return

    # Create Dyskinesia rule..
    with open('dyskinesia.clp', 'r') as file:
        data = file.read()
    logger.info('Creating the Dyskinesia rule')
    response = requests.post(
        url + '/reactive_rule', json={'name': 'dyskinesia', 'content': data})
    if response.status_code != 204:
        logger.error(response.json())
        return

    # Create Excessive heat rule..
    with open('excessive_heat.clp', 'r') as file:
        data = file.read()
    logger.info('Creating the Excessive heat rule')
    response = requests.post(
        url + '/reactive_rule', json={'name': 'excessive_heat', 'content': data})
    if response.status_code != 204:
        logger.error(response.json())
        return

    # Create Fluctuation rule..
    with open('fluctuation.clp', 'r') as file:
        data = file.read()
    logger.info('Creating the Fluctuation rule')
    response = requests.post(
        url + '/reactive_rule', json={'name': 'fluctuation', 'content': data})
    if response.status_code != 204:
        logger.error(response.json())
        return

    # Create Freezing rule..
    with open('freezing.clp', 'r') as file:
        data = file.read()
    logger.info('Creating the Freezing rule')
    response = requests.post(
        url + '/reactive_rule', json={'name': 'freezing', 'content': data})
    if response.status_code != 204:
        logger.error(response.json())
        return

    # Create Mental fatigue rule..
    with open('mental_fatigue.clp', 'r') as file:
        data = file.read()
    logger.info('Creating the Mental fatigue rule')
    response = requests.post(
        url + '/reactive_rule', json={'name': 'mental_fatigue', 'content': data})
    if response.status_code != 204:
        logger.error(response.json())
        return

    # Create Physical fatigue rule..
    with open('physical_fatigue.clp', 'r') as file:
        data = file.read()
    logger.info('Creating the Physical fatigue rule')
    response = requests.post(
        url + '/reactive_rule', json={'name': 'physical_fatigue', 'content': data})
    if response.status_code != 204:
        logger.error(response.json())
        return

    # Create Sensory disregulation rule..
    with open('sensory_disregulation.clp', 'r') as file:
        data = file.read()
    logger.info('Creating the Sensory disregulation rule')
    response = requests.post(
        url + '/reactive_rule', json={'name': 'sensory_disregulation', 'content': data})
    if response.status_code != 204:
        logger.error(response.json())
        return


def create_items(session, url):
    for _ in range(10):
        first_name = fake.first_name()

        response = session.post(url + '/item', json={
            'type': 'User',
            'properties': {
                'name': first_name,
                'baseline_nutrition': fake.boolean(),
                'baseline_fall': fake.random_int(max=10),
                'baseline_freezing': fake.random_int(max=10),
                'baseline_heart_rate': fake.random_int(min=40, max=200),
                'state_anxiety_presence': fake.random_int(max=10),
                'baseline_blood_pressure': fake.random_int(min=60, max=200),
                'stress': fake.random_int(max=10),
            }})

        if response.status_code != 200:
            logger.error('Failed to create item User')
            return
        else:
            logger.info(
                f'Created User {first_name} with id {response.text}')


if __name__ == '__main__':
    url = sys.argv[1] if len(sys.argv) > 1 else 'http://localhost:8080'
    session = requests.Session()

    create_types(session, url)
    create_rules(session, url)
    create_items(session, url)

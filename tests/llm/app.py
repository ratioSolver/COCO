from flask import Flask, request, jsonify
from jinja2 import Template
from dotenv import load_dotenv
import os
from huggingface_hub import InferenceClient
import logging

# Load environment variables from .env file
load_dotenv()

llm_model = os.getenv('LLM_MODEL')
llm_provider = os.getenv('LLM_PROVIDER')
llm_api_key = os.getenv('LLM_API_KEY')

# Initialize the InferenceClient with your Hugging Face API key
client = InferenceClient(provider=llm_provider, api_key=llm_api_key)

app = Flask(__name__)
logging.basicConfig(level=logging.INFO)

logging.info(f"LLM Model: {llm_model}")
logging.info(f"LLM Provider: {llm_provider}")


@app.route('/version')
def version():
    return jsonify({'llm_model': llm_model, 'llm_provider': llm_provider})


@app.route('/understand', methods=['POST'])
def generate():
    data = request.get_json()

    with open('understand.jinja2', 'r') as file:
        template = Template(file.read())
    prompt = template.render(
        slots=data.get('slots', None),
        intents=data.get('intents'),
        entities=data.get('entities'))
    logging.info(f"Prompt: {prompt}")
    messages = [{'role': 'assistant', 'content': prompt}] + \
        data.get('messages')

    completion = client.chat.completions.create(
        model=llm_model, messages=messages)
    return jsonify({'role': completion.choices[0].message.role, 'content': completion.choices[0].message.content})


if __name__ == '__main__':
    app.run(debug=True)

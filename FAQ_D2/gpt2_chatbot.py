from transformers import set_seed
from transformers import pipeline
from transformers import GPT2Tokenizer
from transformers import GPT2LMHeadModel
import sys

response = []
def generate_response(message):
    try:
        generator = pipeline('text-generation', model='gpt2')
        set_seed(42)
        responses = generator(message, max_length=500, num_return_sequences=1, truncation=True)
        response.append(responses)
        return responses[0]['generated_text']
    
    except Exception as e:
        print(f"Error during text generation: {e}")
        return "I'm having trouble understanding that right now."

user_msg = []
if __name__ == "__main__":
    user_message = sys.argv[1] if len(sys.argv) > 1 else "Hello"
    user_msg.append(user_message)
    response = generate_response(user_message)
    print(response)

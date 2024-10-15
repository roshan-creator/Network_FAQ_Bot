import torch
from transformers import GPT2Tokenizer, GPT2LMHeadModel

#Load pre-trained model tokenizer (vocabulary)
tokenizer = GPT2Tokenizer.from_pretrained('gpt2-large')

#Load pre-trained model (weights)
model = GPT2LMHeadModel.from_pretrained('gpt2-large')
model.eval()

# Ensure the model is in evaluation mode
model.eval()

def get_response(message):
    # Encode a text input to a sequence of tokens (numbers)
    inputs = tokenizer.encode(message, return_tensors='pt')

    # Generate a response sequence from the model
    outputs = model.generate(inputs, max_length=100, num_return_sequences=1, no_repeat_ngram_size=2, early_stopping=True)

    # Decode the response to text
    response = tokenizer.decode(outputs[0], skip_special_tokens=True)
    return response

if __name__ == "__main__":
    # Example usage
    test_message = "Hello, who are you?"
    print(get_response(test_message))
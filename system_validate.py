import importlib

required_packages = [
    "pandas",
    "sklearn",
    "nltk",
    "bs4",
    "numpy",
    "scipy",
    "emoji",
    "pickle",
    "ekphrasis",
    "preprocessor"
]

# Check if each package is installed
for package in required_packages:
    try:
        importlib.import_module(package)
        print(f"{package} is installed.")
    except ImportError:
        print(f"{package} is not installed.")



import nltk
from nltk.corpus import stopwords
from nltk.tokenize import word_tokenize
from nltk.stem import WordNetLemmatizer
import string

# Download NLTK resources if not already downloaded
nltk.download('stopwords')
nltk.download('punkt')
nltk.download('wordnet')

try:
    stop_words = set(stopwords.words('english'))
    print("Stopwords are available.")
except LookupError:
    print("Stopwords are not available. Please ensure NLTK resources are downloaded.")

lemmatizer = WordNetLemmatizer()

print("No problem with Lemmatizer, Good to go!!")


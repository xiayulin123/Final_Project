import sounddevice as sd
import soundfile as sf
import requests
import time
import os
from dotenv import load_dotenv
from speechbrain.inference.interfaces import foreign_class

load_dotenv()
API_TOKEN = os.getenv('API_TOKEN')
headers = {"Authorization": f"Bearer {API_TOKEN}"}
API_URL = "https://api-inference.huggingface.co/models/nateraw/hubert-base-emotion"

emotion_to_emoji = {
    "hap": "😄",
    "ang": "😠",
    "sad": "😢",
    "neu": "😐",
    "fea": "😱",
    "sur": "😲",
    "dis": "🤢",
}

def record_audio(filename, duration=3, fs=16000):
    print("Recording...")
    audio = sd.rec(int(duration * fs), samplerate=fs, channels=1, dtype='int16')
    sd.wait()
    sf.write(filename, audio, fs)
    print("Recorded:", filename)

    if not os.path.exists(filename) or os.path.getsize(filename) < 1000:
        print("Recording failed or file is too small.")
        return False
    return True

def query_emotion(audio_path):
    classifier = foreign_class(source="speechbrain/emotion-recognition-wav2vec2-IEMOCAP", pymodule_file="custom_interface.py", classname="CustomEncoderWav2vec2Classifier")
    out_prob, score, index, text_lab = classifier.classify_file(audio_path)
    print(out_prob, score, index, text_lab)
    emoji = emotion_to_emoji.get(text_lab[0], "❓")
    print(f"😶 Emotion: {text_lab[0].upper()} {emoji}")

def main_loop():
    filename = "audio_recording.wav"
    if record_audio(filename):
        query_emotion(filename)

if __name__ == "__main__":
    main_loop()
import requests
import os

file_path = os.path.join(os.getcwd(), "audio_recording.wav")
print(file_path)
if not os.path.exists(file_path):
    print("audio_recording.wav not found in current directory!")
else:
    url = "http://127.0.0.1:9000/predict-emotion/"
    with open(file_path, "rb") as f:
        files = {"file": f}
        print(files)
        response = requests.post(url, files=files)
        print("Response:", response.json())

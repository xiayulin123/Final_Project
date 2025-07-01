from fastapi import FastAPI, File, UploadFile
from fastapi.responses import JSONResponse
from speechbrain.inference.interfaces import foreign_class
import os

app = FastAPI()

emotion_to_emoji = {
    "hap": "😄",
    "ang": "😠",
    "sad": "😢",
    "neu": "😐",
    "fea": "😱",
    "sur": "😲",
    "dis": "🤢",
}

classifier = foreign_class(
    source="speechbrain/emotion-recognition-wav2vec2-IEMOCAP",
    pymodule_file="custom_interface.py",
    classname="CustomEncoderWav2vec2Classifier"
)

@app.post("/predict-emotion/")
async def predict_emotion(file: UploadFile = File(...)):
    try:
        print("Received file:", file.filename)
        temp_dir = os.path.join(os.path.dirname(__file__), "temp")
        os.makedirs(temp_dir, exist_ok=True)
        temp_path = os.path.join(temp_dir, file.filename)

        audio_data = await file.read()
        with open(temp_path, "wb") as f:
            f.write(audio_data)

        print("Saved temp file to:", temp_path)
        out_prob, score, index, text_lab = classifier.classify_file(f"./temp/{file.filename}")
        print("Classification done:", text_lab)

        label_code = text_lab[0]
        emoji = emotion_to_emoji.get(label_code, "❓")

        return {
            "emotion_code": label_code,
            "emoji": emoji,
            "label_full": text_lab
        }

    except Exception as e:
        print("Exception:", e)
        return JSONResponse(status_code=500, content={"error": str(e)})

const express = require('express');
const bodyParser = require('body-parser');
const cors = require('cors');
const multer = require('multer');
const admin = require('firebase-admin');
const fs = require('fs');
const path = require('path');
const os = require('os');

const serviceAccount = require('./serviceAccountKey.json');

admin.initializeApp({
  credential: admin.credential.cert(serviceAccount),
  storageBucket: 'gs://portfolio-73553.appspot.com'
});

const bucket = admin.storage().bucket();
const app = express();
const PORT = 3000;

app.use(cors());
app.use(bodyParser.json());

const upload = multer({ dest: 'uploads/' });

app.post('/esp32', (req, res) => {
  const message = req.body.message;
  console.log(`Received from ESP32: ${message}`);

  // let response = 'Hello!';
  // if (message === 'Hi') response = 'I am Yulin';
  // if (message === 'Hey') response = 'Nice to meet you';

  res.json({ reply: response });
});

app.post('/upload', upload.single('file'), async (req, res) => {
  if (!req.file) {
    return res.status(400).send('No file uploaded.');
  }

  const localPath = req.file.path;
  const firebasePath = `esp32_audio/${Date.now()}_${req.file.originalname || 'record.raw'}`;

  try {
    await bucket.upload(localPath, {
      destination: firebasePath,
      metadata: {
        contentType: 'application/octet-stream'
      }
    });

    // fs.unlinkSync(localPath); // Delete temp file
    console.log(`File uploaded to Firebase: ${firebasePath}`);
    res.send('File uploaded to Firebase Storage.');
  } catch (err) {
    console.error('Upload failed:', err);
    res.status(500).send('Upload failed.');
  }
});

app.post(
  '/upload-raw',
  express.raw({ type: 'application/octet-stream', limit: '20mb' }),
  async (req, res) => {
    try {
      console.log("Called");
      // 1. 构造临时文件路径
      const filename = `${Date.now()}.raw`;
      const tmpPath = path.join(__dirname, 'uploads', filename);

      // 2. 写入本地临时文件
      fs.writeFileSync(tmpPath, req.body);
      console.log(`📥 Received raw file: ${filename}`);

      // 3. 上传到 Firebase
      const firebasePath = `esp32_audio/${filename}`;
      await bucket.upload(tmpPath, {
        destination: firebasePath,
        metadata: {
          contentType: 'application/octet-stream',
        },
      });

      console.log(`Uploaded to Firebase: ${firebasePath}`);

      // 4. 删除本地临时文件
      // fs.unlinkSync(tmpPath);
      console.log('Local file deleted.');

      return res.sendStatus(200);
    } catch (e) {
      console.error('Upload failed:', e);
      return res.status(500).send('Upload failed.');
    }
  }
);

function getLocalIP() {
  const interfaces = os.networkInterfaces();
  for (const name in interfaces) {
    for (const iface of interfaces[name]) {
      if (iface.family === 'IPv4' && !iface.internal) {
        return iface.address;
      }
    }
  }
  return 'localhost';
}

app.listen(PORT, () => {
  const ip = getLocalIP();
  console.log(`🚀 ESP32 server running at http://localhost:${PORT}`);

  console.log(`   🌐 http://${ip}:${PORT}  ← Use this IP on ESP32`);
});

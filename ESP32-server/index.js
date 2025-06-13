const express = require('express');
const bodyParser = require('body-parser');
const cors = require('cors');

const app = express();
app.use(cors());
app.use(bodyParser.json());

const PORT = 3000;

app.post('/esp32', (req, res) => {
  const message = req.body.message;
  console.log(`Received from ESP32: ${message}`);

  let response = 'Hello!';
  if (message === 'Hi') response = 'I am Yulin';
  if (message === 'Hey') response = 'Nice to meet you';

  res.json({ reply: response });
});

app.listen(PORT, () => {
  console.log(`ESP32 server listening on http://localhost:${PORT}`);
});

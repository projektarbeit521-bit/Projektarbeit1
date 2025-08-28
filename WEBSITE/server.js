// server.js
require('dotenv').config();
const express = require('express');
const session = require('express-session');
const cors = require('cors');
const { SerialPort } = require('serialport');
const { ReadlineParser } = require('@serialport/parser-readline');
const qs = require('querystring');

const app = express();
const portHttp = 3001;

const {
  CLIENT_ID,
  REDIRECT_URI,
  SESSION_SECRET
} = process.env;

app.use(cors({
  origin: 'http://localhost:3000',
  credentials: true
}));
app.use(express.json());

// Session (für lokale Entwicklung: secure=false)
app.use(session({
  secret: SESSION_SECRET || 'dev_secret_change_me',
  resave: false,
  saveUninitialized: false,
  cookie: {
    sameSite: 'lax',
    secure: false // für https-Deployment auf true setzen
  }
}));

// --- Hilfs-Middleware: schützt Routen ---
function requireLogin(req, res, next) {
  if (req.session && req.session.user) return next();
  res.status(401).json({ error: 'not_authenticated' });
}

// --- Login: direkte Weiterleitung zu KIT-OIDC ---
app.get('/login', (req, res) => {
  console.log('[login] Handler aktiv');
  const state = Math.random().toString(36).substring(2);
  req.session.oidc_state = state;

  const params = {
    client_id: CLIENT_ID || 'dummy-client',
    response_type: 'code',
    scope: 'openid profile email',
    redirect_uri: REDIRECT_URI || 'http://localhost:3001/callback',
    state
  };

  // Offizieller Authorization-Endpoint des KIT-OIDC (Keycloak):
  const authEndpoint = 'https://oidc.scc.kit.edu/auth/realms/kit/protocol/openid-connect/auth';
  const authUrl = `${authEndpoint}?${qs.stringify(params)}`;

  return res.redirect(authUrl);
});

// --- Callback: zeigt nur Code, kein Token-Tausch ---
app.get('/callback', (req, res) => {
  const { code, state } = req.query;
  if (!code) return res.status(400).send('Kein Code erhalten');
  if (state !== req.session.oidc_state) return res.status(400).send('Ungültiger State');
  delete req.session.oidc_state;

  res.send(`
    <h1>Login erfolgreich!</h1>
    <p>Authorization Code: <code>${code}</code></p>
    <p>(Token-Tausch ist noch nicht implementiert)</p>
  `);
});

// --- Userinfo Route ---
app.get('/userinfo', (req, res) => {
  if (req.session && req.session.user) {
    return res.json({ loggedIn: true, claims: req.session.user.claims || {} });
  }
  res.json({ loggedIn: false });
});

// --- Logout ---
app.post('/logout', (req, res) => {
  req.session.destroy(err => {
    if (err) {
      console.error('Session destroy error:', err);
      return res.status(500).send('Logout failed');
    }
    res.clearCookie('connect.sid');
    res.json({ ok: true });
  });
});

// --- Serielle Verbindung (nur wenn ESP32 dran ist) ---
let portSerial;
try {
  portSerial = new SerialPort({
    path: '/dev/cu.URT0',   // anpassen an dein System
    baudRate: 9600
  });
  const parser = portSerial.pipe(new ReadlineParser({ delimiter: '\r\n' }));
  parser.on('data', line => console.log("ESP32 antwortet:", line));
} catch (err) {
  console.warn("⚠️ Serielle Schnittstelle nicht verfügbar:", err.message);
}

// --- LED Endpunkte ---
app.post('/led/on', requireLogin, (req, res) => {
  if (portSerial) portSerial.write('1', () => console.log("LED an"));
  res.send('LED eingeschaltet');
});

app.post('/led/off', requireLogin, (req, res) => {
  if (portSerial) portSerial.write('0', () => console.log("LED aus"));
  res.send('LED ausgeschaltet');
});

// --- Serverstart ---
app.listen(portHttp, () => {
  console.log(`Server läuft auf http://localhost:${portHttp}`);
});

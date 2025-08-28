// App.js
import './App.css';
import MyCalendar from './MyCalendar';
import Header from './header';
import { useEffect, useState } from 'react';

function App() {
  const [user, setUser] = useState(null);

  // Holt Userinfo beim Laden
  useEffect(() => {
    async function getUser() {
      try {
        const res = await fetch('http://localhost:3001/userinfo', {
          method: 'GET',
          credentials: 'include'
        });
        const data = await res.json();
        if (data.loggedIn) setUser(data.claims);
        else setUser(null);
      } catch (err) {
        console.error('Fehler beim Holen der Userinfo:', err);
      }
    }
    getUser();
  }, []);

  const handleClick = async (state) => {
    const endpoint = `http://localhost:3001/led/${state}`;
    try {
      const res = await fetch(endpoint, { method: 'POST', credentials: 'include' });
      if (res.ok) {
        console.log(`LED ${state}`);
      } else {
        console.error('LED Schalten fehlgeschlagen', await res.text());
      }
    } catch (err) {
      console.error('Fehler beim Schalten der LED:', err);
    }
  };

  const login = () => {
    // leitet auf /login (Backend) weiter, welches zum OIDC Provider redirectet
    window.location.href = 'http://localhost:3001/login';
  };

  const logout = async () => {
    try {
      await fetch('http://localhost:3001/logout', {
        method: 'POST',
        credentials: 'include'
      });
      setUser(null);
    } catch (err) {
      console.error('Logout error', err);
    }
  };

  return (
    <div style={{ padding: '1rem' }}>
      <Header />
      <MyCalendar />

      <div style={{ marginTop: '1rem' }}>
        {user ? (
          <div>
            <div>Angemeldet als: {user.name || user.preferred_username || user.email}</div>
            <button onClick={logout}>Logout</button>
          </div>
        ) : (
          <div>
            <button onClick={login}>Login (KIT/Shibboleth)</button>
          </div>
        )}
      </div>

      <h1 style={{ marginTop: '1.5rem' }}>LED Steuerung</h1>
      <button onClick={() => handleClick('on')}>Lights ON</button>
      <button onClick={() => handleClick('off')}>Lights OFF</button>
      <hr style={{ margin: '2rem 0' }} />
    </div>
  );
}

export default App;



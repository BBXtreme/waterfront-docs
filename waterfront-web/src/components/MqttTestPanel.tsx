// components/MqttTestPanel.tsx
'use client';

import { useState, useEffect } from 'react';
import mqtt, { MqttClient, IClientOptions } from 'mqtt'; // npm install mqtt

const PUBLIC_BROKER = 'wss://broker.emqx.io:8084/mqtt'; // or wss://test.mosquitto.org:8081/mqtt
const LOCAL_BROKER = 'ws://host.docker.internal:9001/mqtt';
const TEST_TOPIC = 'waterfront/test/debug';

export default function MqttTestPanel() {
  const [brokerUrl, setBrokerUrl] = useState(PUBLIC_BROKER);
  const [status, setStatus] = useState<'disconnected' | 'connecting' | 'connected' | 'error'>('disconnected');
  const [logs, setLogs] = useState<string[]>([]);
  const [client, setClient] = useState<MqttClient | null>(null);

  const addLog = (msg: string) => {
    setLogs(prev => [...prev.slice(-15), `${new Date().toLocaleTimeString()} | ${msg}`]);
  };

  const connect = () => {
    if (client) {
      client.end();
      setClient(null);
    }

    setStatus('connecting');
    addLog(`Attempting connection to ${brokerUrl}...`);

    const options: IClientOptions = {
      clientId: `test-pwa-${Math.random().toString(16).slice(3)}`,
      clean: true,
      reconnectPeriod: 3000,       // auto-reconnect
      connectTimeout: 10000,
      keepalive: 60,
    };

    // If using local non-TLS (ws://), allow it only if not on https page
    if (brokerUrl.startsWith('ws://') && window.location.protocol === 'https:') {
      addLog('ERROR: Cannot use ws:// from https:// page (mixed content block)');
      setStatus('error');
      return;
    }

    const mqttClient = mqtt.connect(brokerUrl, options);

    mqttClient.on('connect', () => {
      setStatus('connected');
      addLog('✓ Connected');
      mqttClient.subscribe(TEST_TOPIC, err => {
        if (err) addLog(`Subscribe failed: ${err.message}`);
        else addLog(`Subscribed to ${TEST_TOPIC}`);
      });
    });

    mqttClient.on('message', (topic, message) => {
      addLog(`← Received on ${topic}: ${message.toString()}`);
    });

    mqttClient.on('error', err => {
      addLog(`Connection error: ${err.message || err}`);
      setStatus('error');
    });

    mqttClient.on('reconnect', () => {
      addLog('Reconnecting...');
      setStatus('connecting');
    });

    mqttClient.on('close', () => {
      addLog('Connection closed');
      setStatus('disconnected');
    });

    setClient(mqttClient);

    // Timeout check for slow connections
    setTimeout(() => {
      if (status === 'connecting') {
        addLog('Warning: Connection taking >8s — try public broker or verify Docker config...');
      }
    }, 8000);
  };

  const sendTestMessage = () => {
    if (!client || status !== 'connected') {
      addLog('Cannot send – not connected');
      return;
    }
    const payload = JSON.stringify({
      time: new Date().toISOString(),
      message: 'Test from PWA',
      source: 'test-connection-page',
    });
    client.publish(TEST_TOPIC, payload, { qos: 1 }, err => {
      if (err) addLog(`Publish failed: ${err.message}`);
      else addLog(`→ Published test message`);
    });
  };

  const disconnect = () => {
    if (client) {
      client.end();
      addLog('Disconnected by user');
      setStatus('disconnected');
    }
  };

  const setPublicBroker = () => {
    setBrokerUrl(PUBLIC_BROKER);
    localStorage.setItem('mqttBrokerUrl', PUBLIC_BROKER);
  };

  const setLocalBroker = () => {
    setBrokerUrl(LOCAL_BROKER);
    localStorage.setItem('mqttBrokerUrl', LOCAL_BROKER);
  };

  useEffect(() => {
    const savedUrl = localStorage.getItem('mqttBrokerUrl');
    if (savedUrl) {
      setBrokerUrl(savedUrl);
    }
  }, []);

  useEffect(() => {
    return () => {
      if (client) client.end();
    };
  }, [client]);

  return (
    <div className="p-4 bg-gray-900 text-gray-100 rounded-lg">
      <h2 className="text-xl font-bold mb-4">MQTT Brokers – Test Connection</h2>

      <div className="mb-4">
        <div className="flex gap-2 mb-2">
          <button
            onClick={setPublicBroker}
            className="px-3 py-1 bg-green-700 hover:bg-green-600 rounded text-sm"
          >
            Use Public (stable)
          </button>
          <button
            onClick={setLocalBroker}
            className="px-3 py-1 bg-blue-700 hover:bg-blue-600 rounded text-sm"
          >
            Use Local Docker (macOS)
          </button>
        </div>
        <label className="block mb-1">Broker URL (ws:// or wss://)</label>
        <input
          type="text"
          value={brokerUrl}
          onChange={e => setBrokerUrl(e.target.value)}
          className="w-full p-2 bg-gray-800 border border-gray-700 rounded"
          placeholder="wss://broker.emqx.io:8084/mqtt"
        />
        <p className={`text-sm mt-1 ${brokerUrl.includes('host.docker.internal') ? 'text-red-400' : 'text-yellow-400'}`}>
          {brokerUrl.includes('host.docker.internal')
            ? 'Local Docker selected: ensure mosquitto.conf has "listener 9001" and "protocol websockets", docker-compose.yml has "9001:9001", and container is running. macOS Docker WS often fails — use public if issues.'
            : 'macOS Docker local Mosquitto WS often fails — use public broker (above) for stable test'
          }
        </p>
      </div>

      <div className="flex gap-3 mb-4">
        <button
          onClick={connect}
          disabled={status === 'connecting'}
          className="px-4 py-2 bg-green-700 hover:bg-green-600 rounded disabled:opacity-50"
        >
          Start / Reconnect
        </button>
        <button
          onClick={disconnect}
          disabled={status === 'disconnected'}
          className="px-4 py-2 bg-red-700 hover:bg-red-600 rounded disabled:opacity-50"
        >
          Disconnect
        </button>
        <button
          onClick={sendTestMessage}
          disabled={status !== 'connected'}
          className="px-4 py-2 bg-blue-700 hover:bg-blue-600 rounded disabled:opacity-50"
        >
          Send Test Message
        </button>
      </div>

      <div className="mb-4">
        Status:{' '}
        <span
          className={`font-bold ${
            status === 'connected' ? 'text-green-400' :
            status === 'connecting' ? 'text-yellow-400' :
            status === 'error' ? 'text-red-400' : 'text-gray-400'
          }`}
        >
          {status.toUpperCase()}
        </span>
      </div>

      <div className="bg-black p-3 rounded font-mono text-sm max-h-60 overflow-y-auto">
        {logs.length === 0 ? 'No logs yet...' : logs.map((log, i) => <div key={i}>{log}</div>)}
      </div>
    </div>
  );
}

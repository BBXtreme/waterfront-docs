// components/MqttTestPanel.tsx
'use client';

import { useState, useEffect } from 'react';
import mqtt, { MqttClient, IClientOptions } from 'mqtt'; // npm install mqtt

const PUBLIC_BROKER = 'wss://broker.emqx.io:8084/mqtt'; // or wss://test.mosquitto.org:8081/mqtt
const LOCAL_BROKER = 'ws://host.docker.internal:9001/mqtt';
const HIVEMQ_PUBLIC_BROKER = 'wss://broker.hivemq.com:8884/mqtt';
const HIVEMQ_CLOUD_BROKER = 'wss://8bee884b3e6048c280526f54fe81b9b9.s1.eu.hivemq.cloud:8884/mqtt';
const TEST_TOPIC = 'waterfront/test/debug';

export default function MqttTestPanel() {
  const [brokerUrl, setBrokerUrl] = useState(PUBLIC_BROKER);
  const [status, setStatus] = useState<'disconnected' | 'connecting' | 'connected' | 'error'>('disconnected');
  const [logs, setLogs] = useState<string[]>([]);
  const [client, setClient] = useState<MqttClient | null>(null);
  const [username, setUsername] = useState('');
  const [password, setPassword] = useState('');
  const [showDeepDebug, setShowDeepDebug] = useState(false);

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
      reconnectPeriod: 8000,       // auto-reconnect
      connectTimeout: 10000,
      keepalive: 60,
    };

    if (brokerUrl === HIVEMQ_CLOUD_BROKER) {
      if (username) options.username = username;
      if (password) options.password = password;
    }

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
      addLog(`Error: ${err.message || 'Unknown'} – check browser console for WS details`);
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
        addLog('Warning: Still connecting after 10s – check Docker logs, mosquitto.conf websockets listener, port mapping 9001:9001');
      }
    }, 10000);
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

  const setHivemqPublicBroker = () => {
    setBrokerUrl(HIVEMQ_PUBLIC_BROKER);
    localStorage.setItem('mqttBrokerUrl', HIVEMQ_PUBLIC_BROKER);
  };

  const setHivemqCloudBroker = () => {
    setBrokerUrl(HIVEMQ_CLOUD_BROKER);
    localStorage.setItem('mqttBrokerUrl', HIVEMQ_CLOUD_BROKER);
  };

  const clearLogs = () => {
    setLogs([]);
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
    <div className="p-[25px] bg-gray-900 text-gray-100 rounded-lg">
      <h2 className="text-xl font-bold mb-6">MQTT Brokers – Test Connection (Local, HiveMQ Public, HiveMQ Cloud)</h2>

      <div className="mb-6">
        <div className="flex gap-2 mb-2 flex-wrap">
          <button
            onClick={setPublicBroker}
            className="px-3 py-1 bg-green-700 hover:bg-green-600 rounded text-sm"
          >
            Use EMQX Public
          </button>
          <button
            onClick={setHivemqPublicBroker}
            className="px-3 py-1 bg-blue-700 hover:bg-blue-600 rounded text-sm"
          >
            Use HiveMQ Public
          </button>
          <button
            onClick={setHivemqCloudBroker}
            className="px-3 py-1 bg-purple-700 hover:bg-purple-600 rounded text-sm"
          >
            Use HiveMQ Cloud
          </button>
          <button
            onClick={setLocalBroker}
            className="px-3 py-1 bg-orange-700 hover:bg-orange-600 rounded text-sm"
          >
            Use Local Docker (macOS)
          </button>
        </div>
        <label className="block mb-1">Broker URL (ws:// or wss://)</label>
        <input
          type="text"
          value={brokerUrl}
          onChange={e => setBrokerUrl(e.target.value)}
          className={`w-full p-2 bg-gray-800 border border-gray-700 rounded ${brokerUrl.includes('host.docker.internal') ? 'border-red-500' : ''}`}
          placeholder="wss://broker.emqx.io:8084/mqtt"
        />
        {brokerUrl === HIVEMQ_CLOUD_BROKER && (
          <div className="mt-2 space-y-2">
            <label className="block text-sm">Username</label>
            <input
              type="text"
              value={username}
              onChange={e => setUsername(e.target.value)}
              className="w-full p-2 bg-gray-800 border border-gray-700 rounded"
              placeholder="Enter username"
            />
            <label className="block text-sm">Password</label>
            <input
              type="password"
              value={password}
              onChange={e => setPassword(e.target.value)}
              className="w-full p-2 bg-gray-800 border border-gray-700 rounded"
              placeholder="Enter password"
            />
          </div>
        )}
        {brokerUrl.includes('host.docker.internal') && (
          <div className="text-red-400 bg-red-950/30 p-2 rounded mt-1">
            Local Docker selected: ensure mosquitto.conf has "listener 9001" and "protocol websockets", docker-compose.yml has "9001:9001", and container is running. macOS Docker WS often fails — use public if issues.
          </div>
        )}
        {!brokerUrl.includes('host.docker.internal') && (
          <p className="text-sm text-yellow-400 mt-1">
            macOS Docker local Mosquitto WS often fails — use public broker (above) for stable test
          </p>
        )}
      </div>

      <div className="flex gap-3 mb-6">
        <button
          onClick={connect}
          disabled={status === 'connecting'}
          className="px-4 py-2 bg-green-700 hover:bg-green-600 rounded disabled:opacity-50"
        >
          Start/Reconnect
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
        <button
          onClick={clearLogs}
          className="px-4 py-2 bg-gray-700 hover:bg-gray-600 rounded"
        >
          Clear Logs
        </button>
        <button
          onClick={() => setShowDeepDebug(!showDeepDebug)}
          className="px-4 py-2 bg-purple-700 hover:bg-purple-600 rounded disabled:opacity-50 text-white"
        >
          {showDeepDebug ? 'Hide Deep Debug' : 'Deep Debug'}
        </button>
      </div>

      <div className="mb-6">
        Status:{' '}
        <span
          className={`text-lg font-semibold ${
            status === 'connected' ? 'text-green-400' :
            status === 'connecting' ? 'text-yellow-400' :
            status === 'error' ? 'text-red-400' : 'text-gray-400'
          }`}
        >
          {status.toUpperCase()}
        </span>
      </div>

      <div className="bg-black p-3 rounded font-mono text-sm max-h-80 overflow-y-auto">
        {logs.length === 0 ? 'No logs yet...' : logs.map((log, i) => <div key={i} className="whitespace-pre-wrap break-words">{log}</div>)}
      </div>

      {showDeepDebug && (
        <div className="mt-6 p-4 bg-gray-800 border border-purple-600 rounded-lg text-sm font-mono">
          <h3 className="text-purple-300 font-bold mb-2">Deep Debug Info</h3>
          <pre className="whitespace-pre-wrap break-all">
            Broker URL: {brokerUrl || '(none)'}
            Status: {status}
            Client ID: {client?.options.clientId || '(not connected)'}
            Reconnect period: 10000ms
            Last error: {logs[logs.length-1]?.includes('error') ? logs[logs.length-1] : 'None recent'}
            Browser WS support: {window.WebSocket ? 'Yes' : 'No'}
            Current time: {new Date().toISOString()}
            Connection type: {navigator.connection ? navigator.connection.effectiveType : 'unknown'}
          </pre>
          <button
            onClick={() => {
              addLog('Deep debug snapshot taken at ' + new Date().toLocaleTimeString());
              console.log('MQTT client state:', client);
              console.log('Full logs:', logs);
            }}
            className="mt-3 px-3 py-1 bg-gray-700 hover:bg-gray-600 rounded text-xs"
          >
            Log snapshot to console
          </button>
        </div>
      )}
    </div>
  );
}

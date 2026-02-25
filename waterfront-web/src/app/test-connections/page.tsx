'use client';

import { useState, useEffect } from 'react';
import { createClient } from '@supabase/supabase-js';
import mqtt from 'mqtt';

// Define types for status objects
interface Status {
  status: string;
  message: string;
  timestamp?: string;
}

export default function TestConnectionsPage() {
  const mqttUrls = {
    local: process.env.NEXT_PUBLIC_MQTT_BROKER_URL || 'wss://localhost:9001/mqtt',
    hivemq: 'wss://broker.hivemq.com:8883/mqtt',
    emqx: 'wss://broker.emqx.io:8084/mqtt',
    'hivemq-cloud': 'wss://8bee884b3e6048c280526f54fe81b9b9.s1.eu.hivemq.cloud:8884/mqtt'
  };

  // State for selected broker
  const [selectedBroker, setSelectedBroker] = useState<'local' | 'hivemq' | 'emqx' | 'hivemq-cloud'>('hivemq');

  const activeUrl = mqttUrls[selectedBroker];
  console.log('MQTT URL used:', activeUrl);

  // State for loading
  const [loading, setLoading] = useState(true);

  // State for each status
  const [envStatus, setEnvStatus] = useState<Status>({ status: 'Checking...', message: '' });
  const [supabaseStatus, setSupabaseStatus] = useState<Status>({ status: 'Checking...', message: '' });
  const [mqttStatus, setMqttStatus] = useState<Status>({ status: 'Checking...', message: '' });
  const [vercelStatus, setVercelStatus] = useState<Status>({ status: 'Checking...', message: '' });

  // MQTT client state
  const [mqttClient, setMqttClient] = useState<any>(null);
  const [isMqttConnected, setIsMqttConnected] = useState(false);

  // State for local connection attempts
  const [localConnectionAttempts, setLocalConnectionAttempts] = useState(0);

  // State for credentials
  const [username, setUsername] = useState('');
  const [password, setPassword] = useState('');

  // Load selectedBroker and credentials from localStorage on mount
  useEffect(() => {
    const savedBroker = localStorage.getItem('mqttBrokerChoice');
    if (savedBroker && ['local', 'hivemq', 'emqx', 'hivemq-cloud'].includes(savedBroker)) {
      setSelectedBroker(savedBroker as 'local' | 'hivemq' | 'emqx' | 'hivemq-cloud');
    }
    const savedCredentials = localStorage.getItem('mqttCredentials');
    if (savedCredentials) {
      try {
        const { username: u, password: p } = JSON.parse(savedCredentials);
        setUsername(u || '');
        setPassword(p || '');
      } catch (e) {
        console.error('Failed to parse saved credentials:', e);
      }
    }
  }, []);

  // Save selectedBroker to localStorage
  useEffect(() => {
    localStorage.setItem('mqttBrokerChoice', selectedBroker);
  }, [selectedBroker]);

  // Save credentials to localStorage
  useEffect(() => {
    localStorage.setItem('mqttCredentials', JSON.stringify({ username, password }));
  }, [username, password]);

  // Function to check environment variables
  const checkEnvironment = () => {
    const supabaseUrl = process.env.NEXT_PUBLIC_SUPABASE_URL;
    const supabaseKey = process.env.NEXT_PUBLIC_SUPABASE_ANON_KEY;
    if (supabaseUrl && supabaseKey) {
      setEnvStatus({
        status: 'OK',
        message: `URL: ${supabaseUrl}, Key Length: ${supabaseKey.length}`,
      });
    } else {
      setEnvStatus({
        status: 'Error',
        message: 'Missing SUPABASE_URL or SUPABASE_ANON_KEY',
      });
    }
  };

  // Function to check Supabase connection
  const checkSupabase = async () => {
    try {
      const supabase = createClient(
        process.env.NEXT_PUBLIC_SUPABASE_URL!,
        process.env.NEXT_PUBLIC_SUPABASE_ANON_KEY!
      );
      const { data: session, error } = await supabase.auth.getSession();
      if (error) throw error;
      setSupabaseStatus({
        status: 'Connected (auth OK)',
        message: 'Auth session retrieved successfully',
        timestamp: new Date().toLocaleString(),
      });
    } catch (error: any) {
      setSupabaseStatus({
        status: 'Error ❌',
        message: error.message,
        timestamp: new Date().toLocaleString(),
      });
    }
  };

  // Function to check MQTT connection
  const checkMQTT = () => {
    if (!mqttClient) {
      try {
        console.log(`Connecting to ${selectedBroker} broker: ${activeUrl}`);
        const options: any = {
          protocol: 'wss',
          protocolVersion: 4,
          clean: true,
          reconnectPeriod: 3000,
          connectTimeout: 10000,
          rejectUnauthorized: false,
          clientId: 'waterfront-browser-' + Math.random().toString(16).slice(3),
        };
        if (selectedBroker === 'hivemq-cloud' && username && password) {
          options.username = username;
          options.password = password;
        }
        const client = mqtt.connect(activeUrl, options);
        client.on('connect', () => {
          console.log('MQTT SECURE CONNECTED – protocol:', client.options.protocol);
          setIsMqttConnected(true);
          setMqttStatus({
            status: 'connected',
            message: 'Connected to broker',
            timestamp: new Date().toLocaleString(),
          });
          if (selectedBroker === 'local') {
            setLocalConnectionAttempts(0);
          }
        });
        client.on('error', (error) => {
          console.error('MQTT WS error details:', error);
          setIsMqttConnected(false);
          let errorMessage = 'Connection error: ' + (error.message || error);
          if (error.message && (error.message.includes('lost') || error.message.includes('failed'))) {
            errorMessage = 'Connection lost – check network / broker firewall';
          } else if (selectedBroker === 'hivemq-cloud' && (error.message?.includes('Not authorized') || error.message?.includes('401') || error.message?.includes('403'))) {
            errorMessage = 'Authentication failed – check username/password';
          }
          setMqttStatus({
            status: 'error',
            message: errorMessage,
            timestamp: new Date().toLocaleString(),
          });
          if (selectedBroker === 'local') {
            const newAttempts = localConnectionAttempts + 1;
            setLocalConnectionAttempts(newAttempts);
            if (newAttempts >= 3) {
              setSelectedBroker('hivemq');
              alert('Auto-switched to HiveMQ public due to local connection failure');
            }
          }
        });
        client.on('offline', () => {
          console.log('MQTT OFFLINE');
          setIsMqttConnected(false);
          setMqttStatus({
            status: 'disconnected',
            message: 'Offline – reconnecting...',
            timestamp: new Date().toLocaleString(),
          });
        });
        client.on('close', () => {
          console.log('MQTT CLOSED');
        });
        client.on('reconnect', () => {
          console.log('MQTT reconnect attempt');
        });
        setMqttClient(client);
      } catch (error: any) {
        setMqttStatus({
          status: 'error',
          message: error.message,
          timestamp: new Date().toLocaleString(),
        });
      }
    } else {
      // Update status if already connected
      if (isMqttConnected) {
        setMqttStatus({
          status: 'connected',
          message: 'Connected to broker',
          timestamp: new Date().toLocaleString(),
        });
      }
    }
  };

  // Function to send test message
  const sendTestMessage = () => {
    if (mqttClient && isMqttConnected) {
      const payload = JSON.stringify({
        action: 'test_unlock',
        kayakId: 'test-001',
        broker: selectedBroker,
        timestamp: new Date().toISOString(),
      });
      mqttClient.publish('/kayak/test/unlock', payload, { qos: 1 }, (err) => {
        if (err) {
          console.error('Publish failed:', err);
        } else {
          console.log('Test message published successfully');
        }
      });
    } else {
      alert('MQTT not connected');
    }
  };

  // Function to check Vercel environment
  const checkVercel = () => {
    const vercelEnv = process.env.NEXT_PUBLIC_VERCEL_ENV;
    const vercelUrl = process.env.NEXT_PUBLIC_VERCEL_URL;
    if (vercelEnv || vercelUrl) {
      setVercelStatus({
        status: 'OK',
        message: `Env: ${vercelEnv || 'N/A'}, URL: ${vercelUrl || 'N/A'}`,
      });
    } else {
      setVercelStatus({
        status: 'Not Detected',
        message: 'Not running on Vercel or env vars not set',
      });
    }
  };

  // Function to refresh all statuses
  const refreshStatuses = async () => {
    setLoading(true);
    checkEnvironment();
    await checkSupabase();
    checkMQTT();
    checkVercel();
    setLoading(false);
  };

  // useEffect to run checks on mount
  useEffect(() => {
    refreshStatuses();
  }, []);

  // useEffect for MQTT client cleanup on unmount
  useEffect(() => {
    return () => {
      if (mqttClient) {
        mqttClient.end();
      }
    };
  }, [mqttClient]);

  // useEffect to reconnect when broker changes
  useEffect(() => {
    console.log(`Switched to broker: ${selectedBroker} (${activeUrl})`);
    if (mqttClient) {
      mqttClient.end();
      setMqttClient(null);
      setIsMqttConnected(false);
    }
    if (selectedBroker === 'local') {
      setLocalConnectionAttempts(0);
    }
    checkMQTT();
  }, [selectedBroker]);

  // useEffect for auto-fallback timeout on local
  useEffect(() => {
    if (selectedBroker === 'local') {
      const timer = setTimeout(() => {
        if (!isMqttConnected && selectedBroker === 'local') {
          setSelectedBroker('hivemq');
          alert('Auto-switched to HiveMQ public due to local connection failure');
        }
      }, 10000);
      return () => clearTimeout(timer);
    }
  }, [selectedBroker, isMqttConnected]);

  const brokerLabels = {
    local: 'Local Mosquitto',
    hivemq: 'HiveMQ public',
    emqx: 'EMQX public',
    'hivemq-cloud': 'HiveMQ Cloud (private)'
  };

  return (
    <main className="flex min-h-screen flex-col items-center justify-center p-8 text-center">
      <h1 className="text-4xl font-bold mb-6">Waterfront – Connection & Environment Test</h1>

      {loading && <p className="text-xl mb-4">Loading...</p>}

      <div className="grid grid-cols-1 md:grid-cols-2 gap-6 mb-8">
        {/* Environment Card */}
        <div className="bg-white border border-gray-300 rounded-lg p-6 shadow-md">
          <h2 className="text-2xl font-semibold mb-4">Environment</h2>
          <p className="text-lg">
            Status: <span className={envStatus.status === 'OK' ? 'text-green-600' : 'text-red-600'}>{envStatus.status}</span>
          </p>
          <p className="text-sm text-gray-600">{envStatus.message}</p>
        </div>

        {/* Supabase Card */}
        <div className="bg-white border border-gray-300 rounded-lg p-6 shadow-md">
          <h2 className="text-2xl font-semibold mb-4">Supabase</h2>
          <p className="text-lg">
            Status: <span className={supabaseStatus.status.includes('Connected') ? 'text-green-600' : 'text-red-600'}>{supabaseStatus.status}</span>
          </p>
          <p className="text-sm text-gray-600">{supabaseStatus.message}</p>
          {supabaseStatus.timestamp && <p className="text-xs text-gray-500">Last checked: {supabaseStatus.timestamp}</p>}
        </div>

        {/* MQTT Broker Selector */}
        <div className="bg-white border border-gray-300 rounded-lg p-6 shadow-md col-span-1 md:col-span-2">
          <h2 className="text-2xl font-semibold mb-4">Select MQTT Broker</h2>
          <div className="flex gap-4 flex-wrap">
            {Object.entries(brokerLabels).map(([key, label]) => (
              <label key={key} className="flex items-center cursor-pointer">
                <input
                  type="radio"
                  name="broker"
                  value={key}
                  checked={selectedBroker === key}
                  onChange={(e) => setSelectedBroker(e.target.value as 'local' | 'hivemq' | 'emqx' | 'hivemq-cloud')}
                  className="mr-2"
                />
                <span className="text-sm">{label}</span>
              </label>
            ))}
          </div>
          {selectedBroker === 'local' && (
            <p className="text-red-600 text-sm mt-2">
              Warning: Local Mosquitto WebSocket may not work reliably on macOS Docker. Use HiveMQ or EMQX public for stable browser testing.
            </p>
          )}
          {selectedBroker === 'hivemq-cloud' && (
            <div className="mt-4">
              <div className="mb-2">
                <label className="block text-sm font-medium text-gray-700">Username</label>
                <input
                  type="text"
                  value={username}
                  onChange={(e) => setUsername(e.target.value)}
                  className="mt-1 block w-full px-3 py-2 border border-gray-300 rounded-md shadow-sm focus:outline-none focus:ring-blue-500 focus:border-blue-500"
                  placeholder="Enter username"
                />
              </div>
              <div className="mb-2">
                <label className="block text-sm font-medium text-gray-700">Password</label>
                <input
                  type="password"
                  value={password}
                  onChange={(e) => setPassword(e.target.value)}
                  className="mt-1 block w-full px-3 py-2 border border-gray-300 rounded-md shadow-sm focus:outline-none focus:ring-blue-500 focus:border-blue-500"
                  placeholder="Enter password"
                />
              </div>
              <p className="text-yellow-600 text-sm">
                Credentials stored in browser localStorage – for dev/testing only, not secure for production.
              </p>
            </div>
          )}
        </div>

        {/* MQTT Card */}
        <div className="bg-white border border-gray-300 rounded-lg p-6 shadow-md">
          <h2 className="text-2xl font-bold mb-4">MQTT – {brokerLabels[selectedBroker]}</h2>
          <p className="text-lg">
            Status: <span className={isMqttConnected ? 'text-green-600' : 'text-red-600'}>{mqttStatus.status}</span>
          </p>
          <p className="text-sm text-gray-600">{mqttStatus.message}</p>
          {mqttStatus.timestamp && <p className="text-xs text-gray-500">Last checked: {mqttStatus.timestamp}</p>}
          <button
            onClick={sendTestMessage}
            disabled={!isMqttConnected}
            className="bg-green-600 hover:bg-green-700 text-white px-4 py-2 rounded mt-4"
          >
            Send Test Message
          </button>
        </div>

        {/* Vercel Card */}
        <div className="bg-white border border-gray-300 rounded-lg p-6 shadow-md">
          <h2 className="text-2xl font-semibold mb-4">Vercel</h2>
          <p className="text-lg">
            Status: <span className={vercelStatus.status === 'OK' ? 'text-green-600' : 'text-red-600'}>{vercelStatus.status}</span>
          </p>
          <p className="text-sm text-gray-600">{vercelStatus.message}</p>
        </div>
      </div>

      <button
        onClick={refreshStatuses}
        className="bg-blue-500 hover:bg-blue-700 text-white font-bold py-2 px-4 rounded"
        disabled={loading}
      >
        {loading ? 'Refreshing...' : 'Refresh Status'}
      </button>
    </main>
  );
}

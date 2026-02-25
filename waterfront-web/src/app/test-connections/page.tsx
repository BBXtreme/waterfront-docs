'use client';

import { useState, useEffect } from 'react';
import { createClient } from '@supabase/supabase-js';
import mqtt from 'mqtt';
import { useTheme } from 'next-themes';

// Define types for status objects
interface Status {
  status: string;
  message: string;
  timestamp?: string;
}

export default function TestConnectionsPage() {
  const { theme, setTheme } = useTheme();

  // State for loading
  const [loading, setLoading] = useState(true);

  // State for each status
  const [envStatus, setEnvStatus] = useState<Status>({ status: 'Checking...', message: '' });
  const [supabaseStatus, setSupabaseStatus] = useState<Status>({ status: 'Checking...', message: '' });
  const [vercelStatus, setVercelStatus] = useState<Status>({ status: 'Checking...', message: '' });

  // MQTT states for each broker
  const [localIsStarted, setLocalIsStarted] = useState(false);
  const [localStatus, setLocalStatus] = useState<Status>({ status: 'disconnected', message: 'Not started' });
  const [localClient, setLocalClient] = useState<any>(null);
  const [localIsConnected, setLocalIsConnected] = useState(false);
  const [localAttempts, setLocalAttempts] = useState(0);

  const [hivemqIsStarted, setHivemqIsStarted] = useState(false);
  const [hivemqStatus, setHivemqStatus] = useState<Status>({ status: 'disconnected', message: 'Not started' });
  const [hivemqClient, setHivemqClient] = useState<any>(null);
  const [hivemqIsConnected, setHivemqIsConnected] = useState(false);
  const [hivemqAttempts, setHivemqAttempts] = useState(0);

  const [emqxIsStarted, setEmqxIsStarted] = useState(false);
  const [emqxStatus, setEmqxStatus] = useState<Status>({ status: 'disconnected', message: 'Not started' });
  const [emqxClient, setEmqxClient] = useState<any>(null);
  const [emqxIsConnected, setEmqxIsConnected] = useState(false);
  const [emqxAttempts, setEmqxAttempts] = useState(0);

  const [hivemqCloudIsStarted, setHivemqCloudIsStarted] = useState(false);
  const [hivemqCloudStatus, setHivemqCloudStatus] = useState<Status>({ status: 'disconnected', message: 'Not started' });
  const [hivemqCloudClient, setHivemqCloudClient] = useState<any>(null);
  const [hivemqCloudIsConnected, setHivemqCloudIsConnected] = useState(false);
  const [hivemqCloudAttempts, setHivemqCloudAttempts] = useState(0);

  // State for HiveMQ Cloud credentials
  const [cloudUsername, setCloudUsername] = useState('');
  const [cloudPassword, setCloudPassword] = useState('');

  // Load credentials from localStorage on mount
  useEffect(() => {
    const savedCredentials = localStorage.getItem('mqttCloudCreds');
    if (savedCredentials) {
      try {
        const { username: u, password: p } = JSON.parse(savedCredentials);
        setCloudUsername(u || '');
        setCloudPassword(p || '');
      } catch (e) {
        console.error('Failed to parse saved credentials:', e);
      }
    }
  }, []);

  // Save credentials to localStorage
  useEffect(() => {
    localStorage.setItem('mqttCloudCreds', JSON.stringify({ username: cloudUsername, password: cloudPassword }));
  }, [cloudUsername, cloudPassword]);

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

  // Function to check MQTT connection for a specific broker
  const checkMQTT = (broker: string) => {
    let url = '';
    let options: any = {
      protocolVersion: 4,
      clean: true,
      reconnectPeriod: 3000,
      connectTimeout: 10000,
      rejectUnauthorized: false,
      clientId: `waterfront-browser-${broker}-${Math.random().toString(16).slice(3)}`,
    };

    if (broker === 'local') {
      url = process.env.NEXT_PUBLIC_MQTT_BROKER_URL || 'ws://localhost:9001/mqtt';
      options.protocol = 'ws';
    } else {
      if (broker === 'hivemq') {
        url = 'wss://broker.hivemq.com:8884/mqtt';
        console.log('HiveMQ Public URL:', url);
      } else if (broker === 'emqx') url = 'wss://broker.emqx.io:8084/mqtt';
      else if (broker === 'hivemq-cloud') {
        url = 'wss://8bee884b3e6048c280526f54fe81b9b9.s1.eu.hivemq.cloud:8884/mqtt';
        if (cloudUsername && cloudPassword) {
          options.username = cloudUsername;
          options.password = cloudPassword;
        }
      }
      options.protocol = 'wss';
    }

    console.log(`Connecting to ${broker} broker: ${url}`);
    const client = mqtt.connect(url, options);

    const setStatus = (status: Status) => {
      if (broker === 'local') setLocalStatus(status);
      else if (broker === 'hivemq') setHivemqStatus(status);
      else if (broker === 'emqx') setEmqxStatus(status);
      else if (broker === 'hivemq-cloud') setHivemqCloudStatus(status);
    };

    const setIsConnected = (connected: boolean) => {
      if (broker === 'local') setLocalIsConnected(connected);
      else if (broker === 'hivemq') setHivemqIsConnected(connected);
      else if (broker === 'emqx') setEmqxIsConnected(connected);
      else if (broker === 'hivemq-cloud') setHivemqCloudIsConnected(connected);
    };

    const getAttempts = () => {
      if (broker === 'local') return localAttempts;
      else if (broker === 'hivemq') return hivemqAttempts;
      else if (broker === 'emqx') return emqxAttempts;
      else if (broker === 'hivemq-cloud') return hivemqCloudAttempts;
      return 0;
    };

    const setAttempts = (attempts: number) => {
      if (broker === 'local') setLocalAttempts(attempts);
      else if (broker === 'hivemq') setHivemqAttempts(attempts);
      else if (broker === 'emqx') setEmqxAttempts(attempts);
      else if (broker === 'hivemq-cloud') setHivemqCloudAttempts(attempts);
    };

    const setClient = (c: any) => {
      if (broker === 'local') setLocalClient(c);
      else if (broker === 'hivemq') setHivemqClient(c);
      else if (broker === 'emqx') setEmqxClient(c);
      else if (broker === 'hivemq-cloud') setHivemqCloudClient(c);
    };

    setClient(client);

    client.on('connect', () => {
      console.log(`MQTT SECURE CONNECTED to ${broker} – protocol:`, client.options.protocol);
      setIsConnected(true);
      setStatus({
        status: 'connected',
        message: `Connected to ${url}`,
        timestamp: new Date().toLocaleString(),
      });
      setAttempts(0);
    });

    client.on('error', (error) => {
      console.error(`MQTT WS error for ${broker}:`, error);
      setIsConnected(false);
      let errorMessage = 'Connection error: ' + (error.message || error);
      if (broker === 'hivemq' && error.message && (error.message.includes('lost') || error.message.includes('failed'))) {
        errorMessage = 'Connection failed – check network or try EMQX/HiveMQ Cloud';
      } else if (broker === 'hivemq-cloud' && (error.message?.includes('Not authorized') || error.message?.includes('401') || error.message?.includes('403'))) {
        errorMessage = 'Authentication failed – check username/password';
      }
      setStatus({
        status: 'error',
        message: errorMessage,
        timestamp: new Date().toLocaleString(),
      });
    });

    client.on('offline', () => {
      console.log(`MQTT ${broker} OFFLINE`);
      setIsConnected(false);
      setStatus({
        status: 'disconnected',
        message: 'Offline – reconnecting...',
        timestamp: new Date().toLocaleString(),
      });
    });

    client.on('close', () => {
      console.log(`MQTT ${broker} CLOSED`);
    });

    client.on('reconnect', () => {
      console.log(`MQTT ${broker} reconnect attempt`);
      const newAttempts = getAttempts() + 1;
      setAttempts(newAttempts);
      if (newAttempts > 5) {
        client.end();
        setStatus({
          status: 'error',
          message: 'Max reconnect failed – restart manually',
          timestamp: new Date().toLocaleString(),
        });
      }
    });
  };

  // Start/Stop functions for each broker
  const startLocal = () => {
    setLocalIsStarted(true);
    setLocalStatus({ status: 'connecting', message: 'Connecting...' });
    checkMQTT('local');
  };

  const stopLocal = () => {
    setLocalIsStarted(false);
    if (localClient) localClient.end();
    setLocalClient(null);
    setLocalIsConnected(false);
    setLocalStatus({ status: 'disconnected', message: 'Stopped' });
  };

  const startHivemq = () => {
    setHivemqIsStarted(true);
    setHivemqStatus({ status: 'connecting', message: 'Connecting...' });
    checkMQTT('hivemq');
  };

  const stopHivemq = () => {
    setHivemqIsStarted(false);
    if (hivemqClient) hivemqClient.end();
    setHivemqClient(null);
    setHivemqIsConnected(false);
    setHivemqStatus({ status: 'disconnected', message: 'Stopped' });
  };

  const startEmqx = () => {
    setEmqxIsStarted(true);
    setEmqxStatus({ status: 'connecting', message: 'Connecting...' });
    checkMQTT('emqx');
  };

  const stopEmqx = () => {
    setEmqxIsStarted(false);
    if (emqxClient) emqxClient.end();
    setEmqxClient(null);
    setEmqxIsConnected(false);
    setEmqxStatus({ status: 'disconnected', message: 'Stopped' });
  };

  const startHivemqCloud = () => {
    setHivemqCloudIsStarted(true);
    setHivemqCloudStatus({ status: 'connecting', message: 'Connecting...' });
    checkMQTT('hivemq-cloud');
  };

  const stopHivemqCloud = () => {
    setHivemqCloudIsStarted(false);
    if (hivemqCloudClient) hivemqCloudClient.end();
    setHivemqCloudClient(null);
    setHivemqCloudIsConnected(false);
    setHivemqCloudStatus({ status: 'disconnected', message: 'Stopped' });
  };

  // Send test message functions
  const sendTestMessageLocal = () => {
    if (localClient && localIsConnected) {
      const payload = JSON.stringify({
        action: 'test_unlock',
        kayakId: 'test-001',
        broker: 'local',
        timestamp: new Date().toISOString(),
      });
      localClient.publish('/kayak/test/unlock', payload, { qos: 1 }, (err) => {
        if (err) console.error('Publish failed:', err);
        else console.log('Test message published to local');
      });
    }
  };

  const sendTestMessageHivemq = () => {
    if (hivemqClient && hivemqIsConnected) {
      const payload = JSON.stringify({
        action: 'test_unlock',
        kayakId: 'test-001',
        broker: 'hivemq',
        timestamp: new Date().toISOString(),
      });
      hivemqClient.publish('/kayak/test/unlock', payload, { qos: 1 }, (err) => {
        if (err) console.error('Publish failed:', err);
        else console.log('Test message published to hivemq');
      });
    }
  };

  const sendTestMessageEmqx = () => {
    if (emqxClient && emqxIsConnected) {
      const payload = JSON.stringify({
        action: 'test_unlock',
        kayakId: 'test-001',
        broker: 'emqx',
        timestamp: new Date().toISOString(),
      });
      emqxClient.publish('/kayak/test/unlock', payload, { qos: 1 }, (err) => {
        if (err) console.error('Publish failed:', err);
        else console.log('Test message published to emqx');
      });
    }
  };

  const sendTestMessageHivemqCloud = () => {
    if (hivemqCloudClient && hivemqCloudIsConnected) {
      const payload = JSON.stringify({
        action: 'test_unlock',
        kayakId: 'test-001',
        broker: 'hivemq-cloud',
        timestamp: new Date().toISOString(),
      });
      hivemqCloudClient.publish('/kayak/test/unlock', payload, { qos: 1 }, (err) => {
        if (err) console.error('Publish failed:', err);
        else console.log('Test message published to hivemq-cloud');
      });
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
    if (localIsStarted) checkMQTT('local');
    if (hivemqIsStarted) checkMQTT('hivemq');
    if (emqxIsStarted) checkMQTT('emqx');
    if (hivemqCloudIsStarted) checkMQTT('hivemq-cloud');
    checkVercel();
    setLoading(false);
  };

  // useEffect to run checks on mount
  useEffect(() => {
    refreshStatuses();
  }, []);

  // Cleanup on unmount
  useEffect(() => {
    return () => {
      if (localClient) localClient.end();
      if (hivemqClient) hivemqClient.end();
      if (emqxClient) emqxClient.end();
      if (hivemqCloudClient) hivemqCloudClient.end();
    };
  }, []);

  return (
    <main className="flex min-h-screen flex-col items-center justify-center p-8 text-center relative">
      <button
        onClick={() => setTheme(theme === 'dark' ? 'light' : 'dark')}
        className="absolute top-4 right-4 bg-gray-200 hover:bg-gray-300 dark:bg-gray-700 dark:hover:bg-gray-600 px-4 py-2 rounded"
      >
        Toggle {theme === 'dark' ? 'Light' : 'Dark'} Mode
      </button>

      <h1 className="text-4xl font-bold mb-6">Waterfront – Connection & Environment Test</h1>

      {loading && <p className="text-xl mb-4">Loading...</p>}

      <div className="grid grid-cols-1 md:grid-cols-2 gap-6 mb-8">
        {/* Environment Card */}
        <div className="bg-white dark:bg-gray-800 border border-gray-300 dark:border-gray-600 rounded-lg p-6 shadow-md">
          <h2 className="text-2xl font-semibold mb-4">Environment</h2>
          <p className="text-lg">
            Status: <span className={envStatus.status === 'OK' ? 'text-green-600' : 'text-red-600'}>{envStatus.status}</span>
          </p>
          <p className="text-sm text-gray-600 dark:text-gray-400">{envStatus.message}</p>
        </div>

        {/* Supabase Card */}
        <div className="bg-white dark:bg-gray-800 border border-gray-300 dark:border-gray-600 rounded-lg p-6 shadow-md">
          <h2 className="text-2xl font-semibold mb-4">Supabase</h2>
          <p className="text-lg">
            Status: <span className={supabaseStatus.status.includes('Connected') ? 'text-green-600' : 'text-red-600'}>{supabaseStatus.status}</span>
          </p>
          <p className="text-sm text-gray-600 dark:text-gray-400">{supabaseStatus.message}</p>
          {supabaseStatus.timestamp && <p className="text-xs text-gray-500 dark:text-gray-500">Last checked: {supabaseStatus.timestamp}</p>}
        </div>

        {/* MQTT Broker Cards */}
        <div className="col-span-1 md:col-span-2">
          <div className="flex flex-row gap-4 flex-wrap">
            {/* Local Mosquitto Card */}
            <div className="bg-white dark:bg-gray-800 border border-gray-300 dark:border-gray-600 rounded-lg p-6 shadow-md flex-1 min-w-[300px]">
              <h2 className="text-2xl font-bold mb-4">MQTT - Local Mosquitto</h2>
              <p className="text-lg">
                Status: <span className={localIsConnected ? 'text-green-600' : localStatus.status === 'connecting' ? 'text-yellow-600' : 'text-red-600'}>{localStatus.status}</span>
              </p>
              <p className="text-sm text-gray-600 dark:text-gray-400">{localStatus.message}</p>
              {localStatus.timestamp && <p className="text-xs text-gray-500 dark:text-gray-500">Last checked: {localStatus.timestamp}</p>}
              <p className="text-red-600 text-sm mt-2">Warning: Local WS may fail on macOS Docker – use public for stable test</p>
              <button
                onClick={localIsStarted ? stopLocal : startLocal}
                className={`px-4 py-2 rounded mt-4 ${localIsStarted ? 'bg-red-600 hover:bg-red-700' : 'bg-green-600 hover:bg-green-700'} text-white`}
              >
                {localIsStarted ? 'Stop' : 'Start'}
              </button>
              <button
                onClick={sendTestMessageLocal}
                disabled={!localIsConnected}
                className="bg-blue-600 hover:bg-blue-700 text-white px-4 py-2 rounded mt-4 ml-2 disabled:opacity-50"
              >
                Send Test Message
              </button>
            </div>

            {/* HiveMQ Public Card */}
            <div className="bg-white dark:bg-gray-800 border border-gray-300 dark:border-gray-600 rounded-lg p-6 shadow-md flex-1 min-w-[300px]">
              <h2 className="text-2xl font-bold mb-4">MQTT - HiveMQ Public</h2>
              <p className="text-lg">
                Status: <span className={hivemqIsConnected ? 'text-green-600' : hivemqStatus.status === 'connecting' ? 'text-yellow-600' : 'text-red-600'}>{hivemqStatus.status}</span>
              </p>
              <p className="text-sm text-gray-600 dark:text-gray-400">{hivemqStatus.message}</p>
              {hivemqStatus.timestamp && <p className="text-xs text-gray-500 dark:text-gray-500">Last checked: {hivemqStatus.timestamp}</p>}
              <button
                onClick={hivemqIsStarted ? stopHivemq : startHivemq}
                className={`px-4 py-2 rounded mt-4 ${hivemqIsStarted ? 'bg-red-600 hover:bg-red-700' : 'bg-green-600 hover:bg-green-700'} text-white`}
              >
                {hivemqIsStarted ? 'Stop' : 'Start'}
              </button>
              <button
                onClick={sendTestMessageHivemq}
                disabled={!hivemqIsConnected}
                className="bg-blue-600 hover:bg-blue-700 text-white px-4 py-2 rounded mt-4 ml-2 disabled:opacity-50"
              >
                Send Test Message
              </button>
            </div>

            {/* EMQX Public Card */}
            <div className="bg-white dark:bg-gray-800 border border-gray-300 dark:border-gray-600 rounded-lg p-6 shadow-md flex-1 min-w-[300px]">
              <h2 className="text-2xl font-bold mb-4">MQTT - EMQX Public</h2>
              <p className="text-lg">
                Status: <span className={emqxIsConnected ? 'text-green-600' : emqxStatus.status === 'connecting' ? 'text-yellow-600' : 'text-red-600'}>{emqxStatus.status}</span>
              </p>
              <p className="text-sm text-gray-600 dark:text-gray-400">{emqxStatus.message}</p>
              {emqxStatus.timestamp && <p className="text-xs text-gray-500 dark:text-gray-500">Last checked: {emqxStatus.timestamp}</p>}
              <button
                onClick={emqxIsStarted ? stopEmqx : startEmqx}
                className={`px-4 py-2 rounded mt-4 ${emqxIsStarted ? 'bg-red-600 hover:bg-red-700' : 'bg-green-600 hover:bg-green-700'} text-white`}
              >
                {emqxIsStarted ? 'Stop' : 'Start'}
              </button>
              <button
                onClick={sendTestMessageEmqx}
                disabled={!emqxIsConnected}
                className="bg-blue-600 hover:bg-blue-700 text-white px-4 py-2 rounded mt-4 ml-2 disabled:opacity-50"
              >
                Send Test Message
              </button>
            </div>

            {/* HiveMQ Cloud Card */}
            <div className="bg-white dark:bg-gray-800 border border-gray-300 dark:border-gray-600 rounded-lg p-6 shadow-md flex-1 min-w-[300px]">
              <h2 className="text-2xl font-bold mb-4">MQTT - HiveMQ Cloud (Private)</h2>
              <p className="text-lg">
                Status: <span className={hivemqCloudIsConnected ? 'text-green-600' : hivemqCloudStatus.status === 'connecting' ? 'text-yellow-600' : 'text-red-600'}>{hivemqCloudStatus.status}</span>
              </p>
              <p className="text-sm text-gray-600 dark:text-gray-400">{hivemqCloudStatus.message}</p>
              {hivemqCloudStatus.timestamp && <p className="text-xs text-gray-500 dark:text-gray-500">Last checked: {hivemqCloudStatus.timestamp}</p>}
              <div className="mt-4">
                <div className="mb-2">
                  <label className="block text-sm font-medium text-gray-700 dark:text-gray-300">Username</label>
                  <input
                    type="text"
                    value={cloudUsername}
                    onChange={(e) => setCloudUsername(e.target.value)}
                    className="mt-1 block w-full px-3 py-2 border border-gray-300 dark:border-gray-600 rounded-md shadow-sm focus:outline-none focus:ring-blue-500 focus:border-blue-500 dark:bg-gray-700 dark:text-white"
                    placeholder="Enter username"
                  />
                </div>
                <div className="mb-2">
                  <label className="block text-sm font-medium text-gray-700 dark:text-gray-300">Password</label>
                  <input
                    type="password"
                    value={cloudPassword}
                    onChange={(e) => setCloudPassword(e.target.value)}
                    className="mt-1 block w-full px-3 py-2 border border-gray-300 dark:border-gray-600 rounded-md shadow-sm focus:outline-none focus:ring-blue-500 focus:border-blue-500 dark:bg-gray-700 dark:text-white"
                    placeholder="Enter password"
                  />
                </div>
              </div>
              <button
                onClick={hivemqCloudIsStarted ? stopHivemqCloud : startHivemqCloud}
                className={`px-4 py-2 rounded mt-4 ${hivemqCloudIsStarted ? 'bg-red-600 hover:bg-red-700' : 'bg-green-600 hover:bg-green-700'} text-white`}
              >
                {hivemqCloudIsStarted ? 'Stop' : 'Start'}
              </button>
              <button
                onClick={sendTestMessageHivemqCloud}
                disabled={!hivemqCloudIsConnected}
                className="bg-blue-600 hover:bg-blue-700 text-white px-4 py-2 rounded mt-4 ml-2 disabled:opacity-50"
              >
                Send Test Message
              </button>
            </div>
          </div>
        </div>

        {/* Vercel Card */}
        <div className="bg-white dark:bg-gray-800 border border-gray-300 dark:border-gray-600 rounded-lg p-6 shadow-md">
          <h2 className="text-2xl font-semibold mb-4">Vercel</h2>
          <p className="text-lg">
            Status: <span className={vercelStatus.status === 'OK' ? 'text-green-600' : 'text-red-600'}>{vercelStatus.status}</span>
          </p>
          <p className="text-sm text-gray-600 dark:text-gray-400">{vercelStatus.message}</p>
        </div>
      </div>

      <button
        onClick={refreshStatuses}
        className="bg-blue-500 hover:bg-blue-700 text-white font-bold py-2 px-4 rounded"
        disabled={loading}
      >
        {loading ? 'Refreshing...' : 'Refresh All'}
      </button>
    </main>
  );
}

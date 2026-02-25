'use client';

import { useState, useEffect } from 'react';
import { createClient } from '@supabase/supabase-js';
import mqtt from 'mqtt';

// Optional debug log (can stay here now)
console.log('MQTT_BROKER_URL from env:', process.env.MQTT_BROKER_URL);

// Define types for status objects
interface Status {
  status: string;
  message: string;
  timestamp?: string;
}

export default function TestConnectionsPage() {
  // State for loading
  const [loading, setLoading] = useState(true);

  // State for each status
  const [envStatus, setEnvStatus] = useState<Status>({ status: 'Checking...', message: '' });
  const [supabaseStatus, setSupabaseStatus] = useState<Status>({ status: 'Checking...', message: '' });
  const [mqttStatus, setMqttStatus] = useState<Status>({ status: 'Checking...', message: '' });
  const [vercelStatus, setVercelStatus] = useState<Status>({ status: 'Checking...', message: '' });

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
      // Try a simple query (using a dummy table or auth session)
      const { data, error } = await supabase.from('_dummy').select('count(*)').maybeSingle();
      if (error && error.code !== 'PGRST116') { // PGRST116 is "relation does not exist"
        throw error;
      }
      // Fallback to auth session if query fails
      const { data: session, error: authError } = await supabase.auth.getSession();
      if (authError) throw authError;
      setSupabaseStatus({
        status: 'Connected ✅',
        message: 'Supabase client initialized and query/auth successful',
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
    const brokerUrl = process.env.NEXT_PUBLIC_MQTT_BROKER_URL; // Assuming env var is NEXT_PUBLIC_MQTT_BROKER_URL
    if (!brokerUrl) {
      setMqttStatus({
        status: 'Error',
        message: 'MQTT_BROKER_URL not set',
        timestamp: new Date().toLocaleString(),
      });
      return;
    }
    try {
      const client = mqtt.connect(brokerUrl);
      client.on('connect', () => {
        setMqttStatus({
          status: 'Connected',
          message: 'MQTT client connected',
          timestamp: new Date().toLocaleString(),
        });
        client.end(); // Disconnect after confirming connection
      });
      client.on('error', (error) => {
        setMqttStatus({
          status: 'Error',
          message: error.message,
          timestamp: new Date().toLocaleString(),
        });
      });
      client.on('offline', () => {
        setMqttStatus({
          status: 'Disconnected',
          message: 'MQTT client offline',
          timestamp: new Date().toLocaleString(),
        });
      });
    } catch (error: any) {
      setMqttStatus({
        status: 'Error',
        message: error.message,
        timestamp: new Date().toLocaleString(),
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
    checkMQTT();
    checkVercel();
    setLoading(false);
  };

  // useEffect to run checks on mount
  useEffect(() => {
    refreshStatuses();
  }, []);

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
            Status: <span className={supabaseStatus.status.includes('✅') ? 'text-green-600' : 'text-red-600'}>{supabaseStatus.status}</span>
          </p>
          <p className="text-sm text-gray-600">{supabaseStatus.message}</p>
          {supabaseStatus.timestamp && <p className="text-xs text-gray-500">Last checked: {supabaseStatus.timestamp}</p>}
        </div>

        {/* MQTT Card */}
        <div className="bg-white border border-gray-300 rounded-lg p-6 shadow-md">
          <h2 className="text-2xl font-semibold mb-4">MQTT</h2>
          <p className="text-lg">
            Status: <span className={mqttStatus.status === 'Connected' ? 'text-green-600' : 'text-red-600'}>{mqttStatus.status}</span>
          </p>
          <p className="text-sm text-gray-600">{mqttStatus.message}</p>
          {mqttStatus.timestamp && <p className="text-xs text-gray-500">Last checked: {mqttStatus.timestamp}</p>}
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

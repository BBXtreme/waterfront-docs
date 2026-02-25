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
  const mqttUrl = process.env.NEXT_PUBLIC_MQTT_BROKER_URL || 'mqtt://localhost:1883';
  console.log('MQTT URL:', mqttUrl);

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
        const client = mqtt.connect(mqttUrl, {
          clientId: 'waterfront-test-' + Math.random().toString(16).slice(3),
          reconnect: true,
          clean: true,
        });
        client.on('connect', () => {
          console.log('MQTT connected');
          setIsMqttConnected(true);
          setMqttStatus({
            status: 'Connected',
            message: `Connected to ${mqttUrl}`,
            timestamp: new Date().toLocaleString(),
          });
        });
        client.on('error', (error) => {
          console.log('MQTT error:', error);
          setIsMqttConnected(false);
          setMqttStatus({
            status: 'Error',
            message: error.message + ' Reconnecting...',
            timestamp: new Date().toLocaleString(),
          });
        });
        client.on('offline', () => {
          console.log('MQTT offline');
          setIsMqttConnected(false);
          setMqttStatus({
            status: 'Offline',
            message: 'MQTT client offline',
            timestamp: new Date().toLocaleString(),
          });
        });
        client.on('close', () => {
          console.log('MQTT closed');
        });
        setMqttClient(client);
      } catch (error: any) {
        setMqttStatus({
          status: 'Error',
          message: error.message,
          timestamp: new Date().toLocaleString(),
        });
      }
    } else {
      // Update status if already connected
      if (isMqttConnected) {
        setMqttStatus({
          status: 'Connected',
          message: `Connected to ${mqttUrl}`,
          timestamp: new Date().toLocaleString(),
        });
      }
    }
  };

  // Function to send test message
  const sendTestMessage = () => {
    if (mqttClient && isMqttConnected) {
      const payload = {
        action: 'test_unlock',
        kayakId: 'test-001',
        timestamp: new Date().toISOString(),
      };
      mqttClient.publish('/kayak/test/unlock', JSON.stringify(payload), { qos: 1 }, (err) => {
        if (err) {
          console.error('Publish error:', err);
          alert('Failed to publish message: ' + err.message);
        } else {
          console.log('Published test message');
          alert('Test message sent successfully!');
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

        {/* MQTT Card */}
        <div className="bg-white border border-gray-300 rounded-lg p-6 shadow-md">
          <h2 className="text-2xl font-semibold mb-4">MQTT</h2>
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

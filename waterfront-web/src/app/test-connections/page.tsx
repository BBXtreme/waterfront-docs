'use client';

import { useState, useEffect, useMemo } from 'react';
import { createClient } from '@supabase/supabase-js';
import mqtt from 'mqtt';
import { useTheme } from 'next-themes';
import { Button } from '@/components/ui/button';
import { Input } from '@/components/ui/input';
import { Toggle } from '@/components/ui/toggle';
import { Label } from '@/components/ui/label';
import { toast, Toaster } from 'sonner';
import MachineCard from '@/components/MachineCard';

// Define types for status objects
interface Status {
  status: string;
  message: string;
  timestamp?: string;
}

function TestConnectionsPage() {
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

  // Generate clientId with useMemo to avoid hydration mismatch
  const clientIdSuffix = useMemo(() => Math.random().toString(16).slice(3), []);

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
    console.log('Hydration check: client theme class:', document.documentElement.className);
  }, []);

  // Save credentials to localStorage
  useEffect(() => {
    localStorage.setItem('mqttCloudCreds', JSON.stringify({ username: cloudUsername, password: cloudPassword }));
  }, [cloudUsername, cloudPassword]);

  // Function to insert log into Supabase
  const insertLog = async (topic: string, payload: any, broker: string) => {
    const supabase = createClient(
      process.env.NEXT_PUBLIC_SUPABASE_URL!,
      process.env.NEXT_PUBLIC_SUPABASE_ANON_KEY!
    );
    const { error } = await supabase.from('mqtt_logs').insert({
      broker,
      topic,
      payload
    });
    if (error) throw error;
  };

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
      clientId: `waterfront-browser-${broker}-${clientIdSuffix}`,
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
      // Subscribe to ack topic
      client.subscribe('/kayak/test/ack', (err) => {
        if (err) console.error(`Subscribe error for ${broker}:`, err);
      });
    });

    client.on('message', (topic, message) => {
      if (topic === '/kayak/test/ack') {
        const msg = message.toString();
        console.log(`Ack from ${broker}:`, msg);
        setStatus({
          status: 'connected',
          message: `Ack: ${msg}`,
          timestamp: new Date().toLocaleString(),
        });
      }
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
    toast("Broker Local Mosquitto", { description: "Started" });
  };

  const stopLocal = () => {
    setLocalIsStarted(false);
    if (localClient) localClient.end();
    setLocalClient(null);
    setLocalIsConnected(false);
    setLocalStatus({ status: 'disconnected', message: 'Stopped' });
    toast("Broker Local Mosquitto", { description: "Stopped" });
  };

  const startHivemq = () => {
    setHivemqIsStarted(true);
    setHivemqStatus({ status: 'connecting', message: 'Connecting...' });
    checkMQTT('hivemq');
    toast("Broker HiveMQ Public", { description: "Started" });
  };

  const stopHivemq = () => {
    setHivemqIsStarted(false);
    if (hivemqClient) hivemqClient.end();
    setHivemqClient(null);
    setHivemqIsConnected(false);
    setHivemqStatus({ status: 'disconnected', message: 'Stopped' });
    toast("Broker HiveMQ Public", { description: "Stopped" });
  };

  const startEmqx = () => {
    setEmqxIsStarted(true);
    setEmqxStatus({ status: 'connecting', message: 'Connecting...' });
    checkMQTT('emqx');
    toast("Broker EMQX Public", { description: "Started" });
  };

  const stopEmqx = () => {
    setEmqxIsStarted(false);
    if (emqxClient) emqxClient.end();
    setEmqxClient(null);
    setEmqxIsConnected(false);
    setEmqxStatus({ status: 'disconnected', message: 'Stopped' });
    toast("Broker EMQX Public", { description: "Stopped" });
  };

  const startHivemqCloud = () => {
    setHivemqCloudIsStarted(true);
    setHivemqCloudStatus({ status: 'connecting', message: 'Connecting...' });
    checkMQTT('hivemq-cloud');
    toast("Broker HiveMQ Cloud (Private)", { description: "Started" });
  };

  const stopHivemqCloud = () => {
    setHivemqCloudIsStarted(false);
    if (hivemqCloudClient) hivemqCloudClient.end();
    setHivemqCloudClient(null);
    setHivemqCloudIsConnected(false);
    setHivemqCloudStatus({ status: 'disconnected', message: 'Stopped' });
    toast("Broker HiveMQ Cloud (Private)", { description: "Stopped" });
  };

  // Send test message functions
  const sendTestMessageLocal = async () => {
    if (localClient && localIsConnected) {
      const payloadObject = {
        action: 'test_unlock',
        kayakId: 'test-001',
        broker: 'local',
        timestamp: new Date().toISOString(),
      };
      localClient.publish('/kayak/test/unlock', JSON.stringify(payloadObject), { qos: 1 }, async (err) => {
        if (err) {
          console.error('Publish failed:', err);
          toast.error("Failed", { description: err.message });
        } else {
          console.log('Test message published to local');
          try {
            await insertLog('/kayak/test/unlock', payloadObject, 'local');
            toast.success("Message Sent", { description: `to local` });
          } catch (logErr) {
            console.error('Supabase log failed:', logErr);
            toast.error("Log Failed", { description: 'Supabase log failed' });
          }
        }
      });
    }
  };

  const sendTestMessageHivemq = async () => {
    if (hivemqClient && hivemqIsConnected) {
      const payloadObject = {
        action: 'test_unlock',
        kayakId: 'test-001',
        broker: 'hivemq',
        timestamp: new Date().toISOString(),
      };
      hivemqClient.publish('/kayak/test/unlock', JSON.stringify(payloadObject), { qos: 1 }, async (err) => {
        if (err) {
          console.error('Publish failed:', err);
          toast.error("Failed", { description: err.message });
        } else {
          console.log('Test message published to hivemq');
          try {
            await insertLog('/kayak/test/unlock', payloadObject, 'hivemq');
            toast.success("Message Sent", { description: `to hivemq` });
          } catch (logErr) {
            console.error('Supabase log failed:', logErr);
            toast.error("Log Failed", { description: 'Supabase log failed' });
          }
        }
      });
    }
  };

  const sendTestMessageEmqx = async () => {
    if (emqxClient && emqxIsConnected) {
      const payloadObject = {
        action: 'test_unlock',
        kayakId: 'test-001',
        broker: 'emqx',
        timestamp: new Date().toISOString(),
      };
      emqxClient.publish('/kayak/test/unlock', JSON.stringify(payloadObject), { qos: 1 }, async (err) => {
        if (err) {
          console.error('Publish failed:', err);
          toast.error("Failed", { description: err.message });
        } else {
          console.log('Test message published to emqx');
          try {
            await insertLog('/kayak/test/unlock', payloadObject, 'emqx');
            toast.success("Message Sent", { description: `to emqx` });
          } catch (logErr) {
            console.error('Supabase log failed:', logErr);
            toast.error("Log Failed", { description: 'Supabase log failed' });
          }
        }
      });
    }
  };

  const sendTestMessageHivemqCloud = async () => {
    if (hivemqCloudClient && hivemqCloudIsConnected) {
      const payloadObject = {
        action: 'test_unlock',
        kayakId: 'test-001',
        broker: 'hivemq-cloud',
        timestamp: new Date().toISOString(),
      };
      hivemqCloudClient.publish('/kayak/test/unlock', JSON.stringify(payloadObject), { qos: 1 }, async (err) => {
        if (err) {
          console.error('Publish failed:', err);
          toast.error("Failed", { description: err.message });
        } else {
          console.log('Test message published to hivemq-cloud');
          try {
            await insertLog('/kayak/test/unlock', payloadObject, 'hivemq-cloud');
            toast.success("Message Sent", { description: `to hivemq-cloud` });
          } catch (logErr) {
            console.error('Supabase log failed:', logErr);
            toast.error("Log Failed", { description: 'Supabase log failed' });
          }
        }
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

  const getBadgeVariant = (status: Status, isConnected: boolean) => {
    if (isConnected) return 'default'; // green
    if (status.status === 'connecting') return 'secondary'; // yellow
    if (status.status === 'disconnected' && status.message === 'Stopped') return 'secondary'; // gray
    return 'destructive'; // red
  };

  return (
    <div className="min-h-screen bg-gradient-to-br from-background to-muted/10 p-[50px]">
      {/* Header */}
      <header className="bg-background/80 backdrop-blur-sm border-b sticky top-0 z-10 px-6 py-4 flex justify-between items-center shadow-lg rounded-lg mb-8">
        <h1 className="text-2xl font-semibold">Waterfront – Connection Test</h1>
        <Toggle
          pressed={theme === 'dark'}
          onPressedChange={() => setTheme(theme === 'dark' ? 'light' : 'dark')}
        >
          Dark / Light
        </Toggle>
      </header>

      <main className="max-w-7xl mx-auto">
        {loading && <p className="text-xl text-center">Loading...</p>}

        {/* System Connections */}
        <div className="mb-12">
          <h2 className="text-xl font-medium text-center mb-6">System Connections</h2>
          <div className="grid grid-cols-3 gap-6">
            <MachineCard
              title="Environment"
              status={envStatus.status}
              message={envStatus.message}
              isConnected={envStatus.status === 'OK'}
            />

            <MachineCard
              title="Vercel"
              status={vercelStatus.status}
              message={vercelStatus.message}
              isConnected={vercelStatus.status === 'OK'}
            />

            <MachineCard
              title="Supabase"
              status={supabaseStatus.status}
              message={supabaseStatus.message}
              timestamp={supabaseStatus.timestamp}
              isConnected={supabaseStatus.status.includes('Connected')}
            />
          </div>
        </div>

        {/* MQTT Brokers */}
        <div>
          <h2 className="text-xl font-medium text-center mb-6">MQTT Brokers</h2>
          <div className="grid grid-cols-1 sm:grid-cols-2 lg:grid-cols-4 gap-6">
            {/* Local Mosquitto Card */}
            <MachineCard
              title="MQTT - Local Mosquitto"
              status={localStatus.status}
              message={localStatus.message}
              timestamp={localStatus.timestamp}
              isConnected={localIsConnected}
            >
              <div>
                <p className="text-destructive text-xs">Warning: Local WS may fail on macOS Docker – use public for stable test</p>
              </div>
              <div className="flex gap-4">
                <Button
                  onClick={localIsStarted ? stopLocal : startLocal}
                  variant="outline"
                  className="px-5 py-2.5 mr-2.5"
                >
                  {localIsStarted ? 'Stop' : 'Start'}
                </Button>
                <Button
                  onClick={sendTestMessageLocal}
                  disabled={!localIsConnected}
                  className="px-5 py-2.5 mr-2.5"
                >
                  Send Test Message
                </Button>
              </div>
            </MachineCard>

            {/* HiveMQ Public Card */}
            <MachineCard
              title="MQTT - HiveMQ Public"
              status={hivemqStatus.status}
              message={hivemqStatus.message}
              timestamp={hivemqStatus.timestamp}
              isConnected={hivemqIsConnected}
            >
              <div className="flex gap-4">
                <Button
                  onClick={hivemqIsStarted ? stopHivemq : startHivemq}
                  variant="outline"
                  className="px-5 py-2.5 mr-2.5"
                >
                  {hivemqIsStarted ? 'Stop' : 'Start'}
                </Button>
                <Button
                  onClick={sendTestMessageHivemq}
                  disabled={!hivemqIsConnected}
                  className="px-5 py-2.5 mr-2.5"
                >
                  Send Test Message
                </Button>
              </div>
            </MachineCard>

            {/* EMQX Public Card */}
            <MachineCard
              title="MQTT - EMQX Public"
              status={emqxStatus.status}
              message={emqxStatus.message}
              timestamp={emqxStatus.timestamp}
              isConnected={emqxIsConnected}
            >
              <div className="flex gap-4">
                <Button
                  onClick={emqxIsStarted ? stopEmqx : startEmqx}
                  variant="outline"
                  className="px-5 py-2.5 mr-2.5"
                >
                  {emqxIsStarted ? 'Stop' : 'Start'}
                </Button>
                <Button
                  onClick={sendTestMessageEmqx}
                  disabled={!emqxIsConnected}
                  className="px-5 py-2.5 mr-2.5"
                >
                  Send Test Message
                </Button>
              </div>
            </MachineCard>

            {/* HiveMQ Cloud Card */}
            <MachineCard
              title="MQTT - HiveMQ Cloud (Private)"
              status={hivemqCloudStatus.status}
              message={hivemqCloudStatus.message}
              timestamp={hivemqCloudStatus.timestamp}
              isConnected={hivemqCloudIsConnected}
            >
              <div className="space-y-2">
                <div>
                  <Label htmlFor="username">Username</Label>
                  <Input
                    id="username"
                    type="text"
                    value={cloudUsername}
                    onChange={(e) => setCloudUsername(e.target.value)}
                    placeholder="Enter username"
                  />
                </div>
                <div>
                  <Label htmlFor="password">Password</Label>
                  <Input
                    id="password"
                    type="password"
                    value={cloudPassword}
                    onChange={(e) => setCloudPassword(e.target.value)}
                    placeholder="Enter password"
                  />
                </div>
              </div>
              <div className="flex gap-4">
                <Button
                  onClick={hivemqCloudIsStarted ? stopHivemqCloud : startHivemqCloud}
                  variant="outline"
                  className="px-5 py-2.5 mr-2.5"
                >
                  {hivemqCloudIsStarted ? 'Stop' : 'Start'}
                </Button>
                <Button
                  onClick={sendTestMessageHivemqCloud}
                  disabled={!hivemqCloudIsConnected}
                  className="px-5 py-2.5 mr-2.5"
                >
                  Send Test Message
                </Button>
              </div>
            </MachineCard>
          </div>
        </div>

        {/* Refresh All Button */}
        <div className="text-center mt-8">
          <Button
            onClick={refreshStatuses}
            disabled={loading}
            variant="secondary"
            className="px-5 py-2.5 mr-2.5"
          >
            {loading ? 'Refreshing...' : 'Refresh All'}
          </Button>
        </div>
      </main>
      <Toaster />
    </div>
  );
}

export default function Page() {
  return (
    <TestConnectionsPage />
  );
}

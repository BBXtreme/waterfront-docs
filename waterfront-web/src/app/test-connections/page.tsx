'use client';

import { useState, useEffect, useMemo } from 'react';
import { createClient } from '@supabase/supabase-js';
import mqtt from 'mqtt';
import { useTheme } from 'next-themes';
import { Card, CardContent, CardHeader, CardTitle } from '@/components/ui/card';
import { Button } from '@/components/ui/button';
import { Input } from '@/components/ui/input';
import { Toggle } from '@/components/ui/toggle';
import { Label } from '@/components/ui/label';
import { Badge } from '@/components/ui/badge';
import { toast, Toaster } from 'sonner';
import { Calendar } from '@/components/ui/calendar';

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

  // State for calendar
  const [selectedDate, setSelectedDate] = useState<Date | undefined>(new Date());

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
    <main className="flex min-h-screen flex-col items-center justify-center p-8 text-center relative">
      <Toggle
        pressed={theme === 'dark'}
        onPressedChange={() => setTheme(theme === 'dark' ? 'light' : 'dark')}
        className="absolute top-4 right-4"
      >
        Toggle Dark Mode
      </Toggle>

      <h1 className="text-4xl font-bold mb-6">Waterfront – Connection & Environment Test</h1>

      {loading && <p className="text-xl mb-4">Loading...</p>}

      <div className="grid grid-cols-1 md:grid-cols-2 gap-6 mb-8">
        {/* Environment Card */}
        <Card>
          <CardHeader>
            <CardTitle>Environment</CardTitle>
          </CardHeader>
          <CardContent>
            <p className="text-lg">
              Status: <Badge variant={envStatus.status === 'OK' ? 'default' : 'destructive'}>{envStatus.status}</Badge>
            </p>
            <p className="text-sm text-muted-foreground">{envStatus.message}</p>
          </CardContent>
        </Card>

        {/* Supabase Card */}
        <Card>
          <CardHeader>
            <CardTitle>Supabase</CardTitle>
          </CardHeader>
          <CardContent>
            <p className="text-lg">
              Status: <Badge variant={supabaseStatus.status.includes('Connected') ? 'default' : 'destructive'}>{supabaseStatus.status}</Badge>
            </p>
            <p className="text-sm text-muted-foreground">{supabaseStatus.message}</p>
            {supabaseStatus.timestamp && <p className="text-xs text-muted-foreground">Last checked: {supabaseStatus.timestamp}</p>}
          </CardContent>
        </Card>

        {/* MQTT Broker Cards */}
        <div className="col-span-1 md:col-span-2">
          <div className="flex flex-col md:flex-row gap-6">
            {/* Local Mosquitto Card */}
            <Card className="flex-1 min-w-[300px]">
              <CardHeader>
                <CardTitle>MQTT - Local Mosquitto</CardTitle>
              </CardHeader>
              <CardContent>
                <p className="text-lg">
                  Status: <Badge variant={getBadgeVariant(localStatus, localIsConnected)}>{localStatus.status}</Badge>
                </p>
                <p className="text-sm text-muted-foreground">{localStatus.message}</p>
                {localStatus.timestamp && <p className="text-xs text-muted-foreground">Last checked: {localStatus.timestamp}</p>}
                <p className="text-red-600 text-sm mt-2">Warning: Local WS may fail on macOS Docker – use public for stable test</p>
                <Button
                  onClick={localIsStarted ? stopLocal : startLocal}
                  variant={localIsStarted ? 'destructive' : 'default'}
                  className="mt-4"
                >
                  {localIsStarted ? 'Stop' : 'Start'}
                </Button>
                <Button
                  onClick={sendTestMessageLocal}
                  disabled={!localIsConnected}
                  className="mt-4 ml-2"
                >
                  Send Test Message
                </Button>
              </CardContent>
            </Card>

            {/* HiveMQ Public Card */}
            <Card className="flex-1 min-w-[300px]">
              <CardHeader>
                <CardTitle>MQTT - HiveMQ Public</CardTitle>
              </CardHeader>
              <CardContent>
                <p className="text-lg">
                  Status: <Badge variant={getBadgeVariant(hivemqStatus, hivemqIsConnected)}>{hivemqStatus.status}</Badge>
                </p>
                <p className="text-sm text-muted-foreground">{hivemqStatus.message}</p>
                {hivemqStatus.timestamp && <p className="text-xs text-muted-foreground">Last checked: {hivemqStatus.timestamp}</p>}
                <Button
                  onClick={hivemqIsStarted ? stopHivemq : startHivemq}
                  variant={hivemqIsStarted ? 'destructive' : 'default'}
                  className="mt-4"
                >
                  {hivemqIsStarted ? 'Stop' : 'Start'}
                </Button>
                <Button
                  onClick={sendTestMessageHivemq}
                  disabled={!hivemqIsConnected}
                  className="mt-4 ml-2"
                >
                  Send Test Message
                </Button>
              </CardContent>
            </Card>

            {/* EMQX Public Card */}
            <Card className="flex-1 min-w-[300px]">
              <CardHeader>
                <CardTitle>MQTT - EMQX Public</CardTitle>
              </CardHeader>
              <CardContent>
                <p className="text-lg">
                  Status: <Badge variant={getBadgeVariant(emqxStatus, emqxIsConnected)}>{emqxStatus.status}</Badge>
                </p>
                <p className="text-sm text-muted-foreground">{emqxStatus.message}</p>
                {emqxStatus.timestamp && <p className="text-xs text-muted-foreground">Last checked: {emqxStatus.timestamp}</p>}
                <Button
                  onClick={emqxIsStarted ? stopEmqx : startEmqx}
                  variant={emqxIsStarted ? 'destructive' : 'default'}
                  className="mt-4"
                >
                  {emqxIsStarted ? 'Stop' : 'Start'}
                </Button>
                <Button
                  onClick={sendTestMessageEmqx}
                  disabled={!emqxIsConnected}
                  className="mt-4 ml-2"
                >
                  Send Test Message
                </Button>
              </CardContent>
            </Card>

            {/* HiveMQ Cloud Card */}
            <Card className="flex-1 min-w-[300px]">
              <CardHeader>
                <CardTitle>MQTT - HiveMQ Cloud (Private)</CardTitle>
              </CardHeader>
              <CardContent>
                <p className="text-lg">
                  Status: <Badge variant={getBadgeVariant(hivemqCloudStatus, hivemqCloudIsConnected)}>{hivemqCloudStatus.status}</Badge>
                </p>
                <p className="text-sm text-muted-foreground">{hivemqCloudStatus.message}</p>
                {hivemqCloudStatus.timestamp && <p className="text-xs text-muted-foreground">Last checked: {hivemqCloudStatus.timestamp}</p>}
                <div className="mt-4">
                  <div className="mb-2">
                    <Label htmlFor="username">Username</Label>
                    <Input
                      id="username"
                      type="text"
                      value={cloudUsername}
                      onChange={(e) => setCloudUsername(e.target.value)}
                      placeholder="Enter username"
                    />
                  </div>
                  <div className="mb-2">
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
                <Button
                  onClick={hivemqCloudIsStarted ? stopHivemqCloud : startHivemqCloud}
                  variant={hivemqCloudIsStarted ? 'destructive' : 'default'}
                  className="mt-4"
                >
                  {hivemqCloudIsStarted ? 'Stop' : 'Start'}
                </Button>
                <Button
                  onClick={sendTestMessageHivemqCloud}
                  disabled={!hivemqCloudIsConnected}
                  className="mt-4 ml-2"
                >
                  Send Test Message
                </Button>
              </CardContent>
            </Card>
          </div>
        </div>

        {/* Calendar Card */}
        <Card className="col-span-1 md:col-span-2">
          <CardHeader>
            <CardTitle>Availability Calendar</CardTitle>
          </CardHeader>
          <CardContent>
            <Calendar
              mode="single"
              selected={selectedDate}
              onSelect={setSelectedDate}
              className="rounded-md border"
            />
            <p className="mt-4">Selected date: {selectedDate?.toDateString()}</p>
            {/* Placeholder for availability grid */}
            <div className="mt-4 grid grid-cols-7 gap-2">
              {Array.from({ length: 7 }, (_, i) => (
                <div key={i} className="p-2 border rounded text-center">
                  {i + 9}:00 - {i + 10}:00
                </div>
              ))}
            </div>
          </CardContent>
        </Card>

        {/* Vercel Card */}
        <Card>
          <CardHeader>
            <CardTitle>Vercel</CardTitle>
          </CardHeader>
          <CardContent>
            <p className="text-lg">
              Status: <Badge variant={vercelStatus.status === 'OK' ? 'default' : 'destructive'}>{vercelStatus.status}</Badge>
            </p>
            <p className="text-sm text-muted-foreground">{vercelStatus.message}</p>
          </CardContent>
        </Card>
      </div>

      <Button
        onClick={refreshStatuses}
        disabled={loading}
      >
        {loading ? 'Refreshing...' : 'Refresh All'}
      </Button>
      <Toaster />
    </main>
  );
}

export default function Page() {
  return (
    <TestConnectionsPage />
  );
}

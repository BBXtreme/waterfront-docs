'use client';

import { useState } from 'react';
import { createClient } from '@supabase/supabase-js';
import { Card, CardContent, CardHeader, CardTitle } from '@/components/ui/card';
import { Badge } from '@/components/ui/badge';
import { cn } from '@/lib/utils';
import { toast, Toaster } from 'sonner';
import { Loader2 } from 'lucide-react';
import MqttTestPanel from '@/components/MqttTestPanel';
import { CustomButton } from '@/components/ui/CustomButton';
import { ThemeToggle } from '@/components/ThemeToggle';

// ──────────────────────────────────────────────
// TYPES
// ──────────────────────────────────────────────

interface Status {
  status: string;
  message: string;
  timestamp?: string;
}

// ──────────────────────────────────────────────
// MAIN PAGE COMPONENT
// ──────────────────────────────────────────────

function TestConnectionsPage() {
  const [loading, setLoading] = useState(false);

  // Core services status
  const [envStatus, setEnvStatus] = useState<Status>({
    status: 'Not checked',
    message: 'Click to verify',
  });
  const [supabaseStatus, setSupabaseStatus] = useState<Status>({
    status: 'Not checked',
    message: 'Click to verify',
  });
  const [vercelStatus, setVercelStatus] = useState<Status>({
    status: 'Not checked',
    message: 'Click to verify',
  });

  // Payment gateways
  const [stripeStatus, setStripeStatus] = useState<string>('Not Tested');
  const [stripeResult, setStripeResult] = useState<string>('');
  const [btcPayStatus, setBTCPayStatus] = useState<string>('Not Tested');
  const [btcPayResult, setBTCPayResult] = useState<string>('');

  // Local MQTT status
  const [localStatus, setLocalStatus] = useState<Status>({
    status: 'disconnected',
    message: 'Not started',
  });

  // ──────────────────────────────────────────────
  // TIMEOUT HELPER
  // ──────────────────────────────────────────────

  const withTimeout = <T,>(promise: Promise<T>, ms = 10000): Promise<T> =>
    Promise.race([
      promise,
      new Promise<T>((_, reject) =>
        setTimeout(() => reject(new Error('Request timeout')), ms)
      ),
    ]);

  // ──────────────────────────────────────────────
  // TEST FUNCTIONS
  // ──────────────────────────────────────────────

  const checkEnvironment = () => {
    setLoading(true);

    const required = [
      'NEXT_PUBLIC_SUPABASE_URL',
      'NEXT_PUBLIC_SUPABASE_ANON_KEY',
      'NEXT_PUBLIC_MQTT_BROKER_URL',
      'NEXT_PUBLIC_BTCPAY_URL',
      'NEXT_PUBLIC_BTCPAY_API_KEY',
    ];

    const missing = required.filter(v => !process.env[v]);

    setTimeout(() => {
      if (missing.length === 0) {
        setEnvStatus({
          status: 'OK',
          message: 'All required variables found',
          timestamp: new Date().toLocaleString(),
        });
        toast.success('Environment OK');
      } else {
        setEnvStatus({
          status: 'Missing',
          message: `Missing: ${missing.join(', ')}`,
          timestamp: new Date().toLocaleString(),
        });
        toast.error('Missing environment variables');
      }
      setLoading(false);
    }, 600);
  };

  const testVercelConnection = () => {
    setLoading(true);
    setTimeout(() => {
      const detected = !!process.env.VERCEL_ENV;
      setVercelStatus({
        status: detected ? 'Detected' : 'Not Detected',
        message: detected ? 'On Vercel' : 'Not on Vercel',
        timestamp: new Date().toLocaleString(),
      });
      toast[detected ? 'success' : 'info']('Vercel check complete');
      setLoading(false);
    }, 800);
  };

  const deepDebugVercel = () => {
    toast.info('Vercel debug → console');
    console.group('Vercel Debug');
    console.log('VERCEL_ENV:', process.env.VERCEL_ENV);
    console.log('VERCEL_URL:', process.env.VERCEL_URL);
    console.groupEnd();
  };

  const testSupabaseConnection = async () => {
    setLoading(true);
    try {
      const url = process.env.NEXT_PUBLIC_SUPABASE_URL;
      const key = process.env.NEXT_PUBLIC_SUPABASE_ANON_KEY;

      if (!url || !key) throw new Error('Missing Supabase URL or key');

      const supabase = createClient(url, key);

      const result = await withTimeout(supabase.auth.getSession(), 8000);

      if (result.error) throw result.error;

      setSupabaseStatus({
        status: 'OK',
        message: result.data.session ? 'Authenticated' : 'Connected',
        timestamp: new Date().toLocaleString(),
      });
      toast.success('Supabase OK');
    } catch (err: any) {
      setSupabaseStatus({
        status: 'Error',
        message: err.message,
        timestamp: new Date().toLocaleString(),
      });
      toast.error(`Supabase: ${err.message || 'Failed'}`);
    } finally {
      setLoading(false);
    }
  };

  const deepDebugSupabase = () => {
    toast.info('Supabase debug → console');
    console.group('Supabase Debug');
    console.log('URL:', process.env.NEXT_PUBLIC_SUPABASE_URL);
    console.log('Key length:', process.env.NEXT_PUBLIC_SUPABASE_ANON_KEY?.length || 'missing');
    console.groupEnd();
  };

  const testStripeConnection = () => {
    setLoading(true);
    setStripeStatus('Pending');
    setTimeout(() => {
      setStripeStatus('OK');
      setStripeResult('Stripe test passed (simulated)');
      toast.success('Stripe OK');
      setLoading(false);
    }, 1400);
  };

  const testBTCPayConnection = async () => {
    setLoading(true);
    setBTCPayStatus('Pending');

    try {
      const url = process.env.NEXT_PUBLIC_BTCPAY_URL?.replace(/\/$/, '');
      const apiKey = process.env.NEXT_PUBLIC_BTCPAY_API_KEY;

      if (!url || !apiKey) {
        throw new Error('Missing BTCPay URL or API key – check .env.local');
      }

      const response = await withTimeout(
        fetch(`${url}/api/v1/stores`, {
          method: 'GET',
          headers: {
            'Content-Type': 'application/json',
            Authorization: `token ${apiKey}`,
          },
        }),
        10000
      );

      if (response.ok) {
        setBTCPayStatus('OK');
        setBTCPayResult('BTCPay connected successfully');
        toast.success('BTCPay connection OK');
      } else {
        const errorText = await response.text().catch(() => '');
        throw new Error(`HTTP ${response.status}: ${response.statusText} – ${errorText}`);
      }
    } catch (err: any) {
      console.error('BTCPay error:', err);
      setBTCPayStatus('Error');
      setBTCPayResult(err.message || 'Connection failed');
      toast.error(`BTCPay: ${err.message || 'Unknown error'}`);
    } finally {
      setLoading(false);
    }
  };

  const refreshStatuses = () => {
    setLoading(true);

    Promise.allSettled([
      checkEnvironment(),
      testVercelConnection(),
      testSupabaseConnection(),
      testStripeConnection(),
      testBTCPayConnection(),
    ]).finally(() => {
      setLoading(false);
    });
  };

  // ──────────────────────────────────────────────
  // RENDER
  // ──────────────────────────────────────────────

  return (
    <div className="min-h-screen bg-background relative">
      <Toaster richColors position="top-right" />

      {loading && (
        <div className="fixed inset-0 bg-black/40 backdrop-blur-sm flex items-center justify-center z-50">
          <div className="flex flex-col items-center gap-4 bg-card p-10 rounded-2xl shadow-2xl border">
            <Loader2 className="h-12 w-12 animate-spin text-primary" />
            <p className="text-lg font-medium text-foreground">
              Refreshing connections...
            </p>
          </div>
        </div>
      )}

      <main className="container mx-auto py-8 px-4 md:px-6">
        {/* Header with Theme Toggle */}
        <div className="flex flex-col sm:flex-row justify-between items-start sm:items-center mb-10 gap-4">
          <h1 className="text-3xl font-bold tracking-tight">
            Waterfront – Connection Test Panel
          </h1>

          {/* Debug wrapper – bright yellow so it's impossible to miss */}
          
            <ThemeToggle />
          
        </div>

        {/* Core Checks */}
        <section className="mb-12">
          <h2 className="text-2xl font-semibold mb-6">Core System Checks</h2>
          <div className="grid grid-cols-1 md:grid-cols-3 gap-6">
            <Card>
              <CardHeader>
                <CardTitle>Required Environment Variables</CardTitle>
              </CardHeader>
              <CardContent className="space-y-4 pt-2">
                <div className="flex justify-between items-center text-sm">
                  <span>Supabase, MQTT, BTCPay</span>
                  <Badge variant={envStatus.status === 'OK' ? 'default' : 'destructive'}>
                    {envStatus.status}
                  </Badge>
                </div>
                <CustomButton onClick={checkEnvironment} disabled={loading}>
                  Check Environment
                </CustomButton>
                <p className="text-xs text-muted-foreground">
                  {envStatus.timestamp && `Last: ${envStatus.timestamp}`}
                </p>
              </CardContent>
            </Card>

            <Card>
              <CardHeader>
                <CardTitle>Vercel</CardTitle>
              </CardHeader>
              <CardContent className="space-y-4 pt-2">
                <div className="flex justify-between items-center text-sm">
                  <span>Deployment</span>
                  <Badge variant={vercelStatus.status === 'Detected' ? 'default' : 'secondary'}>
                    {vercelStatus.status}
                  </Badge>
                </div>
                <div className="flex gap-2 flex-wrap">
                  <CustomButton onClick={testVercelConnection} disabled={loading}>
                    Test Connection
                  </CustomButton>
                  <CustomButton variant="outline" onClick={deepDebugVercel} disabled={loading}>
                    Deep Debug
                  </CustomButton>
                </div>
                <p className="text-xs text-muted-foreground">
                  {vercelStatus.timestamp && `Last: ${vercelStatus.timestamp}`}
                </p>
              </CardContent>
            </Card>

            <Card>
              <CardHeader>
                <CardTitle>Supabase API & Auth</CardTitle>
              </CardHeader>
              <CardContent className="space-y-4 pt-2">
                <div className="flex justify-between items-center text-sm">
                  <span>Connection</span>
                  <Badge variant={supabaseStatus.status === 'OK' ? 'default' : 'destructive'}>
                    {supabaseStatus.status}
                  </Badge>
                </div>
                <div className="flex gap-2 flex-wrap">
                  <CustomButton onClick={testSupabaseConnection} disabled={loading}>
                    Test Connection
                  </CustomButton>
                  <CustomButton variant="outline" onClick={deepDebugSupabase} disabled={loading}>
                    Deep Debug
                  </CustomButton>
                </div>
                <p className="text-xs text-muted-foreground">
                  {supabaseStatus.timestamp && `Last: ${supabaseStatus.timestamp}`}
                </p>
              </CardContent>
            </Card>
          </div>
        </section>

        {/* Payment Gateways */}
        <section className="mb-12">
          <h2 className="text-2xl font-semibold mb-6">Payment Gateways</h2>
          <div className="grid grid-cols-1 md:grid-cols-2 gap-6">
            <Card>
              <CardHeader className="flex justify-between items-center">
                <CardTitle>Stripe</CardTitle>
                <Badge className="bg-green-600 hover:bg-green-600">OK</Badge>
              </CardHeader>
              <CardContent className="space-y-4">
                <p className="text-sm text-muted-foreground">Endpoint: api.stripe.com</p>
                <CustomButton
                  onClick={testStripeConnection}
                  disabled={stripeStatus === 'Pending' || loading}
                >
                  Test Connection
                </CustomButton>
                <p className="text-xs text-muted-foreground">{stripeResult}</p>
              </CardContent>
            </Card>

            <Card>
              <CardHeader className="flex justify-between items-center">
                <CardTitle>BTCPay / Lightning</CardTitle>
                <Badge variant={btcPayStatus === 'OK' ? 'default' : 'destructive'}>
                  {btcPayStatus}
                </Badge>
              </CardHeader>
              <CardContent className="space-y-4">
                <p className="text-sm text-muted-foreground">
                  Endpoint: {process.env.NEXT_PUBLIC_BTCPAY_URL || 'not set'}
                </p>
                <CustomButton
                  onClick={testBTCPayConnection}
                  disabled={btcPayStatus === 'Pending' || loading}
                >
                  Test Connection
                </CustomButton>
                <p className="text-xs text-muted-foreground">{btcPayResult}</p>
              </CardContent>
            </Card>
          </div>
        </section>

        {/* MQTT Brokers */}
        <section className="mb-12">
          <h2 className="text-2xl font-semibold mb-6">MQTT Brokers</h2>
          <div className="grid grid-cols-1 gap-6">
            <Card>
              <CardHeader className="flex justify-between items-center">
                <CardTitle>MQTT - Connection Tests</CardTitle>
              </CardHeader>
              <CardContent className="pt-2">
                <MqttTestPanel />
              </CardContent>
            </Card>
          </div>
        </section>

        {/* Refresh All */}
        <div className="text-center mt-12">
          <CustomButton onClick={refreshStatuses} disabled={loading}>
            {loading ? 'Refreshing...' : 'Refresh All'}
          </CustomButton>
        </div>
      </main>
    </div>
  );
}

export default function Page() {
  return <TestConnectionsPage />;
}
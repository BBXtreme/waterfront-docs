export default function AdminLocationsPage() {
  return (
    <div className="bg-wave text-white py-10">
      <div className="max-w-6xl mx-auto px-4 sm:px-6 lg:px-8">
        <h1 className="text-3xl font-bold text-center mb-8">Location Management</h1>
        <div className="grid grid-cols-1 md:grid-cols-2 lg:grid-cols-3 gap-6">
          <Card className="shadow-sm hover:shadow-md transition-shadow rounded-xl border py-6 shadow-sm hover:shadow-md transition-shadow dark:bg-slate-800/60 backdrop-blur">
            <CardHeader className="p-6 pb-0">
              <CardTitle className="font-medium">Compartment 1</CardTitle>
            </CardHeader>
            <CardContent className="p-6">
              <div className="space-y-4">
                <div className="flex justify-between items-center">
                  <span>Status</span>
                  <Badge className="bg-green-100 text-green-700 dark:bg-green-900 dark:text-green-200">Available</Badge>
                </div>
                <div className="flex justify-between items-center">
                  <span>Battery</span>
                  <Badge className="bg-green-100 text-green-700 dark:bg-green-900 dark:text-green-200">95%</Badge>
                </div>
                <Button variant="outline" className="w-full">Unlock</Button>
              </div>
            </CardContent>
          </Card>

          <Card className="shadow-sm hover:shadow-md transition-shadow rounded-xl border py-6 shadow-sm hover:shadow-md transition-shadow dark:bg-slate-800/60 backdrop-blur">
            <CardHeader className="p-6 pb-0">
              <CardTitle className="font-medium">Compartment 2</CardTitle>
            </CardHeader>
            <CardContent className="p-6">
              <div className="space-y-4">
                <div className="flex justify-between items-center">
                  <span>Status</span>
                  <Badge className="bg-amber-100 text-amber-800 dark:bg-amber-900">Booked</Badge>
                </div>
                <div className="flex justify-between items-center">
                  <span>Battery</span>
                  <Badge className="bg-green-100 text-green-700 dark:bg-green-900 dark:text-green-200">88%</Badge>
                </div>
                <Button variant="outline" className="w-full">Unlock</Button>
              </div>
            </CardContent>
          </Card>

          <Card className="shadow-sm hover:shadow-md transition-shadow rounded-xl border py-6 shadow-sm hover:shadow-md transition-shadow dark:bg-slate-800/60 backdrop-blur">
            <CardHeader className="p-6 pb-0">
              <CardTitle className="font-medium">Compartment 3</CardTitle>
            </CardHeader>
            <CardContent className="p-6">
              <div className="space-y-4">
                <div className="flex justify-between items-center">
                  <span>Status</span>
                  <Badge className="bg-red-100 text-red-700 dark:bg-red-900 dark:text-red-200">Overdue</Badge>
                </div>
                <div className="flex justify-between items-center">
                  <span>Battery</span>
                  <Badge className="bg-amber-100 text-amber-800 dark:bg-amber-900">72%</Badge>
                </div>
                <Button variant="destructive" className="w-full">Force Unlock</Button>
              </div>
            </CardContent>
          </Card>
        </div>
      </div>
    </div>
  );
}

// src/app/admin/locations/page.tsx
'use client';

import { useMqtt } from '@/hooks/useMqtt';
import { Card, CardContent, CardHeader, CardTitle } from '@/components/ui/card';
import { Wifi, WifiOff, Battery, AlertCircle } from 'lucide-react';
import { cn } from '@/lib/utils';

export default function LocationsPage() {
  const { isConnected, locationStatuses, error, reconnect } = useMqtt();

  const locations = Object.entries(locationStatuses);

  return (
    <div className="space-y-8">
      <div className="flex items-center justify-between">
        <h1 className="text-3xl font-bold tracking-tight">Locations Overview</h1>

        <div className="flex items-center gap-3">
          <div
            className={cn(
              'h-3 w-3 rounded-full animate-pulse',
              isConnected ? 'bg-green-500' : 'bg-red-500'
            )}
          />
          <span className="text-sm font-medium">
            {isConnected ? 'MQTT Connected' : 'Disconnected'}
          </span>
          {!isConnected && (
            <button
              onClick={reconnect}
              className="text-xs text-blue-600 hover:underline"
            >
              Reconnect
            </button>
          )}
        </div>
      </div>

      {error && (
        <div className="rounded-lg bg-destructive/10 p-4 text-destructive">
          <AlertCircle className="inline h-5 w-5 mr-2" />
          {error}
        </div>
      )}

      <div className="grid gap-6 md:grid-cols-2 lg:grid-cols-3">
        {locations.length === 0 ? (
          <p className="text-muted-foreground col-span-full text-center py-12">
            No location status received yet. Waiting for ESP32 publishes...
          </p>
        ) : (
          locations.map(([locationId, data]) => (
            <Card key={locationId} className="border-border">
              <CardHeader className="pb-3">
                <CardTitle className="flex items-center justify-between text-lg">
                  <span>Location {locationId.slice(0, 8)}...</span>
                  <span
                    className={cn(
                      'inline-flex items-center gap-1.5 text-xs font-medium px-2.5 py-0.5 rounded-full',
                      data.kayakPresent ? 'bg-green-100 text-green-800' : 'bg-amber-100 text-amber-800'
                    )}
                  >
                    {data.kayakPresent ? 'Present' : 'Taken'}
                  </span>
                </CardTitle>
              </CardHeader>
              <CardContent className="space-y-4 text-sm">
                <div className="flex items-center justify-between">
                  <div className="flex items-center gap-2">
                    {data.connType === 'WiFi' ? (
                      <Wifi className="h-4 w-4 text-green-600" />
                    ) : (
                      <WifiOff className="h-4 w-4 text-amber-600" />
                    )}
                    <span>{data.connType}</span>
                  </div>
                  <span className="text-muted-foreground">
                    RSSI: {data.rssi} dBm
                  </span>
                </div>

                <div className="flex items-center justify-between">
                  <div className="flex items-center gap-2">
                    <Battery className="h-4 w-4" />
                    <span>Battery</span>
                  </div>
                  <span className={cn(
                    data.battery > 30 ? 'text-green-600' : 'text-red-600'
                  )}>
                    {data.battery}%
                  </span>
                </div>

                <div className="text-xs text-muted-foreground pt-2 border-t">
                  State: <strong>{data.state}</strong> • Updated just now
                </div>
              </CardContent>
            </Card>
          ))
        )}
      </div>
    </div>
  );
}
// src/app/admin/page.tsx
import { ThemeToggle } from '@/components/ThemeToggle';

export default function AdminOverview() {
  return (
    <div className="min-h-screen bg-background relative">
      {/* Floating toggle – always visible, top-right */}
      <div className="absolute top-4 right-4 z-50">
        <ThemeToggle />
      </div>

      {/* Main admin content */}
      <div className="container mx-auto py-10 px-4 md:px-6">
        <div className="space-y-8">
          <div>
            <h1 className="text-3xl md:text-4xl font-bold tracking-tight text-foreground">
              Welcome to Waterfront Admin
            </h1>
            <p className="mt-3 text-lg text-muted-foreground">
              Use the sidebar to navigate: monitor locations via MQTT, manage bookings, check connections (WiFi/LTE), view BTC/Lightning logs, etc.
            </p>
          </div>

          {/* Optional quick stats or welcome cards */}
          <div className="grid gap-6 md:grid-cols-2 lg:grid-cols-3">
            <div className="rounded-xl border bg-card p-6 shadow-sm">
              <h3 className="text-lg font-medium text-waterfront-primary">Quick Actions</h3>
              <p className="mt-2 text-sm text-muted-foreground">
                Select a section from the left sidebar to begin.
              </p>
            </div>
            {/* Add more teaser cards later */}
          </div>
        </div>
      </div>
    </div>
  );
}
"use client";
import { useState } from 'react';
import MachineCard from '@/components/MachineCard';
import { Button } from '@/components/ui/button';
import Link from 'next/link';

/**
 * AdminDashboardPage component for displaying the admin overview.
 * @returns {JSX.Element} The rendered admin dashboard page.
 */
export default function AdminDashboardPage() {
  return (
    <div className="min-h-screen bg-gradient-to-br from-background to-muted/10 p-[50px]">
      <header className="bg-background/80 backdrop-blur-sm border-b sticky top-0 z-10 px-6 py-4 flex justify-between items-center shadow-lg rounded-lg mb-8">
        <h1 className="text-2xl font-semibold">Admin Dashboard</h1>
        <Link href="/admin/connections">
          <Button variant="outline">Run Connection Tests</Button>
        </Link>
      </header>

      <main className="max-w-7xl mx-auto">
        <h2 className="text-xl font-medium text-center mb-6">Overview</h2>
        <div className="grid grid-cols-1 md:grid-cols-3 gap-2.5">
          <MachineCard
            title="Machine 1"
            status="connected"
            message="Operational"
            timestamp="2023-10-01 12:00:00"
            isConnected={true}
          >
            <div className="flex gap-4">
              <Button className="px-5 py-2.5 mr-2.5">Action</Button>
            </div>
          </MachineCard>

          <MachineCard
            title="Machine 2"
            status="disconnected"
            message="Offline"
            timestamp="2023-10-01 11:00:00"
            isConnected={false}
          >
            <div className="flex gap-4">
              <Button className="px-5 py-2.5 mr-2.5">Action</Button>
            </div>
          </MachineCard>

          <MachineCard
            title="Machine 3"
            status="connecting"
            message="Reconnecting..."
            timestamp="2023-10-01 10:00:00"
            isConnected={false}
          >
            <div className="flex gap-4">
              <Button className="px-5 py-2.5 mr-2.5">Action</Button>
            </div>
          </MachineCard>
        </div>
      </main>
    </div>
  );
}

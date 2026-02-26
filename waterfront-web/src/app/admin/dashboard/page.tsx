import MachineCard from '@/components/MachineCard';

export default function AdminDashboardPage() {
  return (
    <div className="min-h-screen bg-gradient-to-br from-background to-muted/10 p-[50px]">
      <header className="bg-background/80 backdrop-blur-sm border-b sticky top-0 z-10 px-6 py-4 flex justify-between items-center shadow-lg rounded-lg mb-8">
        <h1 className="text-2xl font-semibold">Admin Dashboard</h1>
      </header>

      <main className="max-w-7xl mx-auto">
        <h2 className="text-xl font-medium text-center mb-6">Overview</h2>
        <div className="grid grid-cols-1 md:grid-cols-3 gap-6">
          <MachineCard
            title="Machine 1"
            status="connected"
            message="Operational"
            timestamp="2023-10-01 12:00:00"
            isConnected={true}
          >
            <div className="flex gap-3">
              <button className="px-5 py-2.5 bg-primary text-primary-foreground rounded">Action</button>
            </div>
          </MachineCard>

          <MachineCard
            title="Machine 2"
            status="disconnected"
            message="Offline"
            timestamp="2023-10-01 11:00:00"
            isConnected={false}
          >
            <div className="flex gap-3">
              <button className="px-5 py-2.5 bg-primary text-primary-foreground rounded">Action</button>
            </div>
          </MachineCard>

          <MachineCard
            title="Machine 3"
            status="connecting"
            message="Reconnecting..."
            timestamp="2023-10-01 10:00:00"
            isConnected={false}
          >
            <div className="flex gap-3">
              <button className="px-5 py-2.5 bg-primary text-primary-foreground rounded">Action</button>
            </div>
          </MachineCard>
        </div>
      </main>
    </div>
  );
}

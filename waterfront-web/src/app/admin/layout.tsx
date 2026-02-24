import { redirect } from 'next/navigation';
import { createServerClient } from '@/lib/supabase/server';
import Link from 'next/link';

export default async function AdminLayout({
  children,
}: {
  children: React.ReactNode;
}) {
  const supabase = createServerClient();
  const { data: { session } } = await supabase.auth.getSession();

  if (!session) {
    redirect('/admin/login');
  }

  return (
    <div className="flex h-screen bg-gray-100">
      {/* Sidebar */}
      <aside className="w-64 bg-white shadow-md">
        <div className="p-4">
          <h1 className="text-xl font-bold">Waterfront Admin</h1>
        </div>
        <nav className="mt-4">
          <ul>
            <li>
              <Link href="/admin/dashboard" className="block px-4 py-2 hover:bg-gray-200">
                Dashboard
              </Link>
            </li>
            <li>
              <Link href="/admin/bookings" className="block px-4 py-2 hover:bg-gray-200">
                Bookings
              </Link>
            </li>
            <li>
              <Link href="/admin/machines" className="block px-4 py-2 hover:bg-gray-200">
                Machines & Telemetry
              </Link>
            </li>
            <li>
              <Link href="/admin/logs" className="block px-4 py-2 hover:bg-gray-200">
                Logs & Events
              </Link>
            </li>
          </ul>
        </nav>
      </aside>

      {/* Main Content */}
      <div className="flex-1 flex flex-col">
        {/* Top Bar */}
        <header className="bg-white shadow-sm p-4 flex justify-between items-center">
          <h2 className="text-lg font-semibold">Waterfront Admin</h2>
          <div className="flex items-center space-x-4">
            <span>{session.user.email}</span>
            <button className="bg-red-500 text-white px-4 py-2 rounded">Logout</button>
          </div>
        </header>

        {/* Page Content */}
        <main className="flex-1 p-6">
          {children}
        </main>
      </div>
    </div>
  );
}

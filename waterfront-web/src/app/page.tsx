"use client";

import Link from "next/link";

export default function Home() {
  return (
    <div style={{ display: 'flex', flexDirection: 'column', alignItems: 'center', justifyContent: 'center', minHeight: '100vh', padding: '2rem', textAlign: 'center' }}>
      <h1 style={{ fontSize: '2.5rem', fontWeight: 'bold', marginBottom: '1.5rem' }}>Waterfront – Dev Start</h1>
      <p style={{ fontSize: '1.25rem', marginBottom: '2.5rem', maxWidth: '42rem' }}>
        Minimal setup • Nordend / Frankfurt • Connection tests first
      </p>
      <Link
        href="/test-connections"
        className="bg-blue-600 text-white px-8 py-4 rounded-full text-lg font-medium shadow-lg hover:bg-blue-700 transition-colors"
        onMouseOver={(e) => e.currentTarget.classList.add('bg-blue-700')}
        onMouseOut={(e) => e.currentTarget.classList.remove('bg-blue-700')}
      >
        → Start Connection Tests
      </Link>
      <Link
        href="/booking"
        className="bg-green-600 text-white px-8 py-4 rounded-full text-lg font-medium shadow-lg hover:bg-green-700 transition-colors mt-4"
        onMouseOver={(e) => e.currentTarget.classList.add('bg-green-700')}
        onMouseOut={(e) => e.currentTarget.classList.remove('bg-green-700')}
      >
        → Start Booking Tests
      </Link>
    </div>
  );
}

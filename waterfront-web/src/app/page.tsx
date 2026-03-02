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
        style={{ backgroundColor: '#2563eb', color: 'white', padding: '1rem 2rem', borderRadius: '0.5rem', textDecoration: 'none', fontSize: '1.125rem', fontWeight: '500', boxShadow: '0 4px 6px -1px rgba(0, 0, 0, 0.1)', transition: 'background-color 0.2s' }}
        onMouseOver={(e) => e.currentTarget.style.backgroundColor = '#1d4ed8'}
        onMouseOut={(e) => e.currentTarget.style.backgroundColor = '#2563eb'}
      >
        → Start Connection Tests
      </Link>
    </div>
  );
}

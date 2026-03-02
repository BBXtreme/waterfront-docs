"use client";

import Link from "next/link";
import { Button } from '@/components/ui/button';

export default function Home() {
  return (
    <div style={{ display: 'flex', flexDirection: 'column', alignItems: 'center', justifyContent: 'center', minHeight: '100vh', padding: '2rem', textAlign: 'center' }}>
      <h1 style={{ fontSize: '2.5rem', fontWeight: 'bold', marginBottom: '1.5rem' }}>Waterfront – Dev Start</h1>
      <p style={{ fontSize: '1.25rem', marginBottom: '2.5rem', maxWidth: '42rem' }}>
        Minimal setup • Nordend / Frankfurt • Connection tests first
      </p>
      <Link href="/test-connections">
        <Button className="bg-blue-600 text-white px-8 py-4 rounded-full text-lg font-medium shadow-lg hover:bg-blue-700 transition-colors">
          → Start Connection Tests
        </Button>
      </Link>
      <Link href="/booking">
        <Button className="bg-green-600 text-white px-8 py-4 rounded-full text-lg font-medium shadow-lg hover:bg-green-700 transition-colors mt-4">
          → Start Booking Tests
        </Button>
      </Link>
    </div>
  );
}

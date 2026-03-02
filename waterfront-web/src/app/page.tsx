"use client";

import Link from "next/link";
import { Button } from '@/components/ui/button';

export default function Home() {
  return (
    <div className="min-h-screen bg-background flex flex-col items-center justify-center p-8">
      <h1 className="text-4xl font-bold mb-6 text-center">Waterfront – Dev Start</h1>
      <p className="text-lg mb-8 text-center max-w-md text-muted-foreground">
        Minimal setup • Nordend / Frankfurt • Connection tests first
      </p>
      <div className="space-y-4">
        <Link href="/test-connections">
          <Button className="bg-waterfront-primary text-white px-8 py-4 rounded-full text-lg font-medium shadow-sm hover:bg-waterfront-primary/90 transition-colors">
            → Start Connection Tests
          </Button>
        </Link>
        <Link href="/booking">
          <Button className="bg-secondary text-secondary-foreground px-8 py-4 rounded-full text-lg font-medium shadow-sm hover:bg-secondary/80 transition-colors">
            → Start Booking Tests
          </Button>
        </Link>
      </div>
    </div>
  );
}

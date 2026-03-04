// src/app/page.tsx
import Link from 'next/link';
import { Button } from '@/components/ui/button';
import { ThemeToggle } from '@/components/ThemeToggle'; // ← single import, no duplicate

const buttonStyle = `
  w-full
  max-w-xs
  bg-gray-200
  hover:bg-white
  active:bg-gray-50
  text-gray-900
  border border-gray-300
  rounded-full
  px-10 py-3.5
  text-md font-normal
  shadow-sm hover:shadow
  transition-all duration-250 ease-out
  focus:outline-none focus:ring-2 focus:ring-gray-400 focus:ring-offset-2
`;

export default function Home() {
  return (
    // Outer container with relative positioning → allows absolute toggle
    <div className="min-h-screen bg-wave text-white relative">
      {/* Floating toggle – top-right, always visible */}
      <div className="absolute top-4 right-4 z-50">
        <ThemeToggle />
      </div>

      {/* Main content – centered */}
      <div className="flex flex-col items-center justify-center min-h-screen py-16 md:py-24 px-6">
        <div className="text-center max-w-4xl">
          <h1 className="text-5xl md:text-6xl lg:text-7xl font-bold tracking-tight mb-6">
            Waterfront Kayak Rental
          </h1>
          <p className="text-xl md:text-2xl mb-12 text-white/90 max-w-2xl mx-auto">
            24/7 self-service booking • Bremen Harbor & surroundings<br />
            Pay with card, Apple Pay or Lightning BTC
          </p>
        </div>

        <div className="grid grid-cols-1 sm:grid-cols-2 md:grid-cols-3 gap-6 w-full max-w-5xl">
          <Link href="/booking">
            <Button size="lg" className={buttonStyle}>
              Start Booking
            </Button>
          </Link>

          <Link href="/payment">
            <Button size="lg" className={buttonStyle}>
              Explore Crypto Payment
            </Button>
          </Link>

          <Link href="/admin">
            <Button size="lg" className={buttonStyle}>
              Admin Panel
            </Button>
          </Link>

          <Link href="/pwa">
            <Button size="lg" className={buttonStyle}>
              Customer App (PWA)
            </Button>
          </Link>

          <Link href="/test-connections">
            <Button size="lg" className={buttonStyle}>
              Test Connections
            </Button>
          </Link>
        </div>
      </div>
    </div>
  );
}
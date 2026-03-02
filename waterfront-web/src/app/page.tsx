import Link from "next/link";

export default function Home() {
  return (
    <div className="flex min-h-screen flex-col items-center justify-center p-8 text-center bg-gradient-to-br from-background to-muted/10">
      <h1 className="text-4xl font-bold mb-6">Waterfront – Dev Start</h1>
      <p className="text-xl mb-10 max-w-2xl">
        Minimal setup • Nordend / Frankfurt • Connection tests first
      </p>
      <div className="flex gap-4 flex-wrap justify-center">
        <Link
          href="/test-connections"
          className="bg-blue-600 hover:bg-blue-700 text-white px-8 py-4 rounded-lg text-lg font-medium shadow-md transition-colors"
        >
          → Connection Tests
        </Link>
        <Link
          href="/booking"
          className="bg-green-600 hover:bg-green-700 text-white px-8 py-4 rounded-lg text-lg font-medium shadow-md transition-colors"
        >
          → Book a Kayak
        </Link>
      </div>
    </div>
  );
}

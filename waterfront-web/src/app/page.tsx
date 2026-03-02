import Link from "next/link";
import { Button } from "@/components/ui/button";

export default function Home() {
  return (
    <div className="flex min-h-screen flex-col items-center justify-center p-8 text-center bg-gradient-to-br from-background to-muted/10">
      <h1 className="text-4xl font-bold mb-6">Waterfront – Dev Start</h1>
      <p className="text-xl mb-10 max-w-2xl">
        Minimal setup • Nordend / Frankfurt • Connection tests first
      </p>
      <div className="flex gap-4 flex-wrap justify-center">
        <Button asChild size="lg" className="px-8 py-4 text-lg">
          <Link href="/test-connections">
            → Connection Tests
          </Link>
        </Button>
        <Button asChild size="lg" variant="secondary" className="px-8 py-4 text-lg">
          <Link href="/booking">
            → Book a Kayak
          </Link>
        </Button>
      </div>
    </div>
  );
}

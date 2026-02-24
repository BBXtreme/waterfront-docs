import { Button } from "@/components/ui/button";
import { Card } from "@/components/ui/card";
import Link from "next/link";

export default function Home() {
  return (
    <main className="min-h-screen flex flex-col items-center justify-center bg-white px-5 py-10">
      <div className="w-full max-w-md space-y-10 text-center">
        <div>
          <h1 className="text-5xl font-bold tracking-tight text-gray-900 sm:text-6xl">
            Book Kayak or SUP
          </h1>
          <p className="mt-6 text-xl text-gray-600">
            In minutes – no login, no app download
          </p>
        </div>

        <Button asChild size="lg" className="w-full h-20 text-2xl rounded-2xl bg-blue-600 hover:bg-blue-700">
          <Link href="/booking">
            Book now →
          </Link>
        </Button>

        <Card className="p-6 bg-gray-50 border-none shadow-sm">
          <p className="text-lg font-medium">Bremen Waterfront Locations</p>
          <p className="text-sm text-gray-500 mt-2">
            Select your spot on next screen
          </p>
        </Card>

        <p className="text-sm text-gray-500">
          Powered by solar-powered smart lockers • Pay with card or Bitcoin
        </p>
      </div>
    </main>
  );
}
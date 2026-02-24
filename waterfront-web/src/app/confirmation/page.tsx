import { Button } from "@/components/ui/button";
import Image from "next/image";
import { Card } from "@/components/ui/card";

export default function ConfirmationPage({
  params,
}: {
  params: { bookingId: string };
}) {
  // Fake for now – later: fetch real PIN/QR from Supabase after payment
  const fakePin = "748259"; // 6-digit
  const qrData = `WATERFRONT:${params.bookingId}:${fakePin}`;
  const qrUrl = `https://api.qrserver.com/v1/create-qr-code/?size=300x300&data=${encodeURIComponent(qrData)}`;

  return (
    <main className="min-h-screen bg-white flex flex-col items-center justify-center px-6 py-10">
      <div className="w-full max-w-md space-y-10 text-center">
        <h1 className="text-5xl font-bold text-green-700">Booking Confirmed!</h1>

        <Card className="p-8 border-2 border-green-200 bg-green-50">
          <p className="text-2xl font-semibold mb-6">Your access code</p>

          <div className="bg-white p-6 rounded-xl shadow-inner mx-auto max-w-[320px]">
            <Image
              src={qrUrl}
              alt="QR Code for locker"
              width={300}
              height={300}
              className="mx-auto"
              priority
            />
          </div>

          <div className="mt-8">
            <p className="text-xl mb-3">Or enter manually:</p>
            <div className="text-5xl font-mono tracking-widest font-bold">
              {fakePin}
            </div>
          </div>
        </Card>

        <div className="space-y-6 text-lg">
          <p>
            Go to the machine → scan QR or enter code → compartment opens
          </p>
          <p className="font-medium">
            Return the equipment to the same bay before end time
          </p>
        </div>

        <Button variant="outline" size="lg" className="w-full h-16 text-xl">
          Add to Home Screen (PWA)
        </Button>

        <p className="text-sm text-gray-500">
          Booking ID: {params.bookingId} • Enjoy your time on the water!
        </p>
      </div>
    </main>
  );
}
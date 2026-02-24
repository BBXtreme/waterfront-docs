"use client";

import { useState } from "react";
import { Calendar } from "@/components/ui/calendar";
import { Button } from "@/components/ui/button";
import { Card, CardContent, CardHeader, CardTitle } from "@/components/ui/card";
import {
  Select,
  SelectContent,
  SelectItem,
  SelectTrigger,
  SelectValue,
} from "@/components/ui/select";

export default function BookingPage() {
  const [date, setDate] = useState<Date | undefined>(new Date());
  const [duration, setDuration] = useState("1"); // Default 1 hour
  const [location, setLocation] = useState("bremen-harbor");

  const handlePay = () => {
    // Fake redirect for now – later: API call to Supabase/Stripe/BTCPay → MQTT unlock
    window.location.href = `/confirmation/demo-${Date.now().toString(36)}`;
  };

  return (
    <main className="min-h-screen bg-white px-5 py-8 flex flex-col">
      <div className="max-w-md mx-auto w-full space-y-8 flex-1">
        <h1 className="text-4xl font-bold text-center">Book Your Slot</h1>

        <Card className="border-none shadow-lg">
          <CardHeader>
            <CardTitle className="text-2xl">Select Date</CardTitle>
          </CardHeader>
          <CardContent>
            <Calendar
              mode="single"
              selected={date}
              onSelect={setDate}
              disabled={(d) => d < new Date() || d > new Date(Date.now() + 1000 * 60 * 60 * 24 * 30)} // Up to 30 days ahead
              className="rounded-md border mx-auto"
            />
          </CardContent>
        </Card>

        <Card className="border-none shadow-lg">
          <CardHeader>
            <CardTitle className="text-2xl">Duration</CardTitle>
          </CardHeader>
          <CardContent>
            <Select value={duration} onValueChange={setDuration}>
              <SelectTrigger className="h-14 text-xl">
                <SelectValue placeholder="Select hours" />
              </SelectTrigger>
              <SelectContent>
                <SelectItem value="1">1 hour – €15</SelectItem>
                <SelectItem value="1.5">1.5 hours – €20</SelectItem>
                <SelectItem value="2">2 hours – €27</SelectItem>
                <SelectItem value="3">3 hours – €34</SelectItem>
                <SelectItem value="4">4 hours – €41</SelectItem>
              </SelectContent>
            </Select>
          </CardContent>
        </Card>

        <Card className="border-none shadow-lg">
          <CardHeader>
            <CardTitle className="text-2xl">Location</CardTitle>
          </CardHeader>
          <CardContent>
            <Select value={location} onValueChange={setLocation}>
              <SelectTrigger className="h-14 text-xl">
                <SelectValue placeholder="Select location" />
              </SelectTrigger>
              <SelectContent>
                <SelectItem value="bremen-harbor">Bremen Harbor</SelectItem>
                <SelectItem value="weser-promenade">Weser Promenade</SelectItem>
                {/* Add more as needed */}
              </SelectContent>
            </Select>
          </CardContent>
        </Card>

        <Button
          onClick={handlePay}
          className="w-full h-20 text-3xl rounded-2xl mt-8 bg-blue-600 hover:bg-blue-700"
          size="lg"
        >
          Pay & Get Code →
        </Button>

        <p className="text-center text-sm text-gray-500">
          Secure payment • Instant QR & code
        </p>
      </div>
    </main>
  );
}
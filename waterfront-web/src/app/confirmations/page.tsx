'use client';

import { useSearchParams } from 'next/navigation';
import { Card, CardContent, CardHeader, CardTitle } from '@/components/ui/card';
import { Badge } from '@/components/ui/badge';
import { Button } from '@/components/ui/button';
import { ThemeToggle } from '@/components/ThemeToggle';

export default function ConfirmationPage() {
  const searchParams = useSearchParams();
  const name = searchParams.get('name') || 'N/A';
  const email = searchParams.get('email') || 'N/A';
  const date = searchParams.get('date') || 'N/A';
  const time = searchParams.get('time') || 'N/A';
  const kayakType = searchParams.get('kayakType') || 'N/A';

  // Mock PIN for demo (in real app, generate or fetch from backend)
  const pinCode = '1234';

  return (
    <div className="min-h-screen bg-background py-16 px-8">
      <div className="max-w-4xl mx-auto text-center space-y-8">
        {/* Success Banner */}
        <div className="bg-green-100 dark:bg-green-900/40 text-green-800 dark:text-green-200 p-6 rounded-xl">
          <Badge className="mb-4 bg-green-100 text-green-700 dark:bg-green-900 dark:text-green-200">Booking Confirmed</Badge>
          <p className="text-lg">Thank you for your booking! Here are the details:</p>
        </div>

        {/* Booking Details */}
        <Card className="shadow-sm hover:shadow-md transition-shadow rounded-xl border py-6 shadow-sm hover:shadow-md transition-shadow dark:bg-slate-800/50 backdrop-blur-sm">
          <CardHeader className="p-6 pb-0">
            <CardTitle className="text-4xl md:text-5xl font-bold tracking-tight text-center">Booking Confirmation</CardTitle>
          </CardHeader>
          <CardContent className="p-6 space-y-6">
            <div className="space-y-2">
              <p><strong>Name:</strong> {name}</p>
              <p><strong>Email:</strong> {email}</p>
              <p><strong>Date:</strong> {date}</p>
              <p><strong>Time:</strong> {time}</p>
              <p><strong>Kayak Type:</strong> {kayakType}</p>
            </div>
            <p className="text-sm text-muted-foreground">
              You will receive a confirmation email shortly. Please arrive 15 minutes early for check-in.
            </p>
          </CardContent>
        </Card>

        {/* PIN Display */}
        <Card className="shadow-lg rounded-2xl border-2 border-waterfront-primary p-8">
          <CardHeader className="p-0 pb-4">
            <CardTitle className="text-2xl font-semibold text-center">Your Access PIN</CardTitle>
          </CardHeader>
          <CardContent className="p-0">
            <div className="text-6xl font-bold tracking-widest text-waterfront-primary font-mono">
              {pinCode}
            </div>
            <p className="text-sm text-muted-foreground mt-4">Use this PIN to unlock your kayak at the machine.</p>
          </CardContent>
        </Card>

        {/* QR Code Placeholder */}
        <Card className="shadow-lg rounded-2xl border-2 border-waterfront-primary p-8">
          <CardHeader className="p-0 pb-4">
            <CardTitle className="text-2xl font-semibold text-center">QR Code</CardTitle>
          </CardHeader>
          <CardContent className="p-0">
            <div className="w-48 h-48 bg-gray-200 dark:bg-gray-700 mx-auto rounded-lg flex items-center justify-center">
              <span className="text-muted-foreground">QR Code Placeholder</span>
            </div>
            <p className="text-sm text-muted-foreground mt-4">Scan this QR code at the machine for quick access.</p>
          </CardContent>
        </Card>

        {/* Action Buttons */}
        <div className="flex flex-col sm:flex-row gap-4 justify-center">
          <Button variant="default" size="lg">Download Receipt</Button>
          <Button variant="outline" size="lg">Share Booking</Button>
        </div>
      </div>
    </div>
  );
}

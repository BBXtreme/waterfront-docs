'use client';

import { useSearchParams } from 'next/navigation';
import { Card, CardContent, CardHeader, CardTitle } from '@/components/ui/card';
import { Badge } from '@/components/ui/badge';

export default function ConfirmationPage() {
  const searchParams = useSearchParams();
  const name = searchParams.get('name') || 'N/A';
  const email = searchParams.get('email') || 'N/A';
  const date = searchParams.get('date') || 'N/A';
  const time = searchParams.get('time') || 'N/A';
  const kayakType = searchParams.get('kayakType') || 'N/A';

  return (
    <div className="min-h-screen bg-background p-8">
      <div className="max-w-2xl mx-auto">
        <Card className="shadow-sm rounded-lg overflow-hidden bg-card border">
          <CardHeader className="p-6 pb-0">
            <CardTitle className="font-medium text-center">Booking Confirmation</CardTitle>
          </CardHeader>
          <CardContent className="p-6 space-y-4">
            <div className="text-center">
              <Badge className="mb-4 bg-green-100 text-green-700 dark:bg-green-900 dark:text-green-200">Booking Confirmed</Badge>
              <p className="text-muted-foreground">Thank you for your booking! Here are the details:</p>
            </div>
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
      </div>
    </div>
  );
}

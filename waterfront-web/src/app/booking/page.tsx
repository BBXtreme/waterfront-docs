'use client';

import { useState } from 'react';
import { useRouter } from 'next/navigation';
import { createClient } from '@supabase/supabase-js';
import { Button } from '@/components/ui/button';
import { Input } from '@/components/ui/input';
import { Label } from '@/components/ui/label';
import { Card, CardContent, CardHeader, CardTitle } from '@/components/ui/card';
import { toast, Toaster } from 'sonner';

export default function BookingPage() {
  const router = useRouter();
  const [name, setName] = useState('');
  const [email, setEmail] = useState('');
  const [date, setDate] = useState('');
  const [time, setTime] = useState('');
  const [kayakType, setKayakType] = useState('single');

  const handleSubmit = async (e: React.FormEvent) => {
    e.preventDefault();

    // Create Supabase client
    const supabase = createClient(
      process.env.NEXT_PUBLIC_SUPABASE_URL!,
      process.env.NEXT_PUBLIC_SUPABASE_ANON_KEY!
    );

    try {
      // Insert booking into Supabase
      const { error } = await supabase.from('bookings').insert({
        name,
        email,
        date,
        time,
        kayak_type: kayakType,
        created_at: new Date().toISOString(),
      });

      if (error) {
        throw error;
      }

      toast.success('Booking submitted!', {
        description: `Name: ${name}, Date: ${date}, Time: ${time}, Type: ${kayakType}`,
      });

      // Navigate to confirmation with query params
      const params = new URLSearchParams({
        name,
        email,
        date,
        time,
        kayakType,
      });
      router.push(`/confirmation?${params.toString()}`);
    } catch (error: any) {
      console.error('Booking submission failed:', error?.message || error);
      toast.error('Booking failed', {
        description: error?.message || 'Please try again later.',
      });
    }
  };

  return (
    <div className="min-h-screen bg-gradient-to-br from-background to-muted/10 p-[50px]">
      <div className="max-w-2xl mx-auto">
        <Card className="shadow-sm rounded-lg overflow-hidden bg-card border border-border">
          <CardHeader className="p-[25px] pb-0">
            <CardTitle className="font-medium text-center">Book Your Kayak</CardTitle>
          </CardHeader>
          <CardContent className="p-[25px]">
            <form onSubmit={handleSubmit} className="space-y-6">
              <div className="space-y-2">
                <Label htmlFor="name">Full Name</Label>
                <Input
                  id="name"
                  type="text"
                  value={name}
                  onChange={(e) => setName(e.target.value)}
                  placeholder="Enter your full name"
                  required
                />
              </div>
              <div className="space-y-2">
                <Label htmlFor="email">Email</Label>
                <Input
                  id="email"
                  type="email"
                  value={email}
                  onChange={(e) => setEmail(e.target.value)}
                  placeholder="Enter your email"
                  required
                />
              </div>
              <div className="space-y-2">
                <Label htmlFor="date">Date</Label>
                <Input
                  id="date"
                  type="date"
                  value={date}
                  onChange={(e) => setDate(e.target.value)}
                  required
                />
              </div>
              <div className="space-y-2">
                <Label htmlFor="time">Time</Label>
                <select
                  id="time"
                  value={time}
                  onChange={(e) => setTime(e.target.value)}
                  className="w-full p-2 border border-input bg-background rounded-md"
                  required
                >
                  <option value="">Select a time</option>
                  <option value="09:00">9:00 AM</option>
                  <option value="10:00">10:00 AM</option>
                  <option value="11:00">11:00 AM</option>
                  <option value="12:00">12:00 PM</option>
                  <option value="13:00">1:00 PM</option>
                  <option value="14:00">2:00 PM</option>
                  <option value="15:00">3:00 PM</option>
                  <option value="16:00">4:00 PM</option>
                  <option value="17:00">5:00 PM</option>
                  <option value="18:00">6:00 PM</option>
                </select>
              </div>
              <div className="space-y-2">
                <Label htmlFor="kayakType">Kayak Type</Label>
                <select
                  id="kayakType"
                  value={kayakType}
                  onChange={(e) => setKayakType(e.target.value)}
                  className="w-full p-2 border border-input bg-background rounded-md"
                >
                  <option value="single">Single Kayak</option>
                  <option value="double">Double Kayak</option>
                  <option value="canoe">Canoe</option>
                </select>
              </div>
              <Button type="submit" className="w-full bg-white text-black px-4 py-2 rounded-full text-base font-medium shadow-md transition-colors">
                Submit Booking
              </Button>
            </form>
          </CardContent>
        </Card>
      </div>
      <Toaster />
    </div>
  );
}

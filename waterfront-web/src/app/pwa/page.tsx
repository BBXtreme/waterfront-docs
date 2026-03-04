'use client';

import { useState } from 'react';
import { useRouter } from 'next/navigation';
import { createClient } from '@supabase/supabase-js';
import { Button } from '@/components/ui/button';
import { Input } from '@/components/ui/input';
import { Label } from '@/components/ui/label';
import { Card, CardContent, CardHeader, CardTitle } from '@/components/ui/card';
import { toast, Toaster } from 'sonner';
import { ThemeToggle } from '@/components/ThemeToggle';

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
    <div className="min-h-screen bg-waterfront-primary/10 dark:bg-waterfront-primary/20 p-8">
      <div className="max-w-2xl mx-auto">
        <Card className="shadow-sm hover:shadow-md transition-shadow rounded-xl border py-6 shadow-sm hover:shadow-md transition-shadow dark:bg-slate-800/50 backdrop-blur-sm">
          <CardHeader className="p-6 pb-0">
            <h1 className="text-4xl md:text-5xl font-bold tracking-tight text-center">Book Your Kayak</h1>
          </CardHeader>
          <CardContent className="p-6">
            <form onSubmit={handleSubmit} className="space-y-8">
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
                  className="w-full p-2 border border-input bg-background rounded-md focus-visible:ring-2 focus-visible:ring-waterfront-primary focus-visible:border-waterfront-primary"
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
                  className="w-full p-2 border border-input bg-background rounded-md focus-visible:ring-2 focus-visible:ring-waterfront-primary focus-visible:border-waterfront-primary"
                >
                  <option value="single">Single Kayak</option>
                  <option value="double">Double Kayak</option>
                  <option value="canoe">Canoe</option>
                </select>
              </div>
              <Button type="submit" variant="default" size="lg" className="transition-all duration-200 ease-in-out">
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

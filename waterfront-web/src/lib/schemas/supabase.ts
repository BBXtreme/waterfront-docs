import { z } from 'zod';

// Example for a 'bookings' table row from Supabase
export const bookingRowSchema = z.object({
  id: z.string().uuid(),
  user_id: z.string().uuid().nullable(),
  location_slug: z.string(),
  compartment_id: z.number().int().positive(),
  start_time: z.string().datetime(),
  end_time: z.string().datetime(),
  status: z.enum(['pending', 'confirmed', 'active', 'completed', 'cancelled']),
  payment_method: z.enum(['stripe', 'btc']),
  payment_status: z.enum(['pending', 'succeeded', 'failed']),
  created_at: z.string().datetime(),
  updated_at: z.string().datetime(),
});

export const bookingRowListSchema = z.array(bookingRowSchema);

export type BookingRow = z.infer<typeof bookingRowSchema>;
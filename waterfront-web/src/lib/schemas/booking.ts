import { z } from 'zod';

export const bookingSchema = z.object({
  locationSlug: z.string().min(1, 'Location is required'),
  compartmentId: z.number().int().positive('Invalid compartment'),
  startDate: z.date().refine((date) => date > new Date(), {
    message: 'Start date must be in the future',
  }),
  durationHours: z
    .number()
    .int('Duration must be whole hours')
    .min(1, 'Minimum 1 hour')
    .max(48, 'Maximum 48 hours'),
  paymentMethod: z.enum(['stripe', 'btc'], {
    required_error: 'Please select a payment method',
  }),
  guestName: z.string().min(2, 'Name must be at least 2 characters').optional(), // guest mode
  guestEmail: z.string().email('Invalid email').optional(),
}).refine(
  (data) => {
    // Example business rule: BTC bookings might require longer min duration
    if (data.paymentMethod === 'btc' && data.durationHours < 2) {
      return false;
    }
    return true;
  },
  {
    message: 'BTC payments require minimum 2 hours',
    path: ['durationHours'],
  }
);

export type Booking = z.infer<typeof bookingSchema>;
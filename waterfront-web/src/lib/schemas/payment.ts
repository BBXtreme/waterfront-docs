import { z } from 'zod';

export const paymentSuccessSchema = z.object({
  bookingId: z.string().uuid(),
  paymentId: z.string().min(1),
  amount: z.number().positive(),
  currency: z.enum(['EUR', 'USD', 'BTC', 'USDT']),
  method: z.enum(['stripe', 'btcpay']),
  status: z.literal('succeeded'),
  timestamp: z.string().datetime(),
});

export type PaymentSuccess = z.infer<typeof paymentSuccessSchema>;
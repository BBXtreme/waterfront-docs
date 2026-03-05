import { z } from 'zod';

const envSchema = z.object({
  NEXT_PUBLIC_SUPABASE_URL: z.string().url({ message: 'Invalid Supabase URL' }),
  NEXT_PUBLIC_SUPABASE_ANON_KEY: z.string().min(20, 'Supabase anon key is missing or too short'),

  // HiveMQ Cloud credentials (used in frontend MQTT client)
  NEXT_PUBLIC_MQTT_BROKER_URL: z
    .string()
    .url()
    .refine((url) => url.startsWith('mqtts://'), {
      message: 'MQTT broker must use secure protocol (mqtts://)',
    }),
  NEXT_PUBLIC_MQTT_USERNAME: z.string().min(1, 'MQTT username required'),
  NEXT_PUBLIC_MQTT_PASSWORD: z.string().min(1, 'MQTT password required'),

  // Optional – Stripe / BTCPay public keys if used client-side
  NEXT_PUBLIC_STRIPE_PUBLISHABLE_KEY: z.string().optional(),
  NEXT_PUBLIC_BTCPAY_URL: z.string().url().optional(),
});

export const env = envSchema.parse(process.env);

// Optional: type export for convenience
export type Env = z.infer<typeof envSchema>;
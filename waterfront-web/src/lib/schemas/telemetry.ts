import { z } from 'zod';

export const telemetrySummarySchema = z.object({
  locationSlug: z.string(),
  totalCompartments: z.number().int().nonnegative(),
  available: z.number().int().nonnegative(),
  occupied: z.number().int().nonnegative(),
  lowBattery: z.number().int().nonnegative(),
  lastUpdate: z.string().datetime(),
  averageBattery: z.number().min(0).max(100),
});

export type TelemetrySummary = z.infer<typeof telemetrySummarySchema>;
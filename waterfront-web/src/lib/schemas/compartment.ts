import { z } from 'zod';

export const compartmentSchema = z.object({
  id: z.number().int().positive(),
  locationSlug: z.string().min(1),
  name: z.string().min(1, 'Compartment name required'),
  status: z.enum(['available', 'occupied', 'maintenance', 'reserved']),
  batteryLevel: z.number().min(0).max(100).optional(),
  lastUpdated: z.string().datetime().optional(),
});

export const compartmentListSchema = z.array(compartmentSchema);

export type Compartment = z.infer<typeof compartmentSchema>;
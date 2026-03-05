import { z } from 'zod';

// Status update from a single compartment
export const mqttStatusSchema = z.object({
  compartmentId: z.number().int().positive(),
  locked: z.boolean(),
  sensorOccupied: z.boolean(),
  battery: z.number().min(0).max(100),
  timestamp: z.string().datetime(),
  error: z.string().nullable().optional(),
});

export const mqttTelemetrySchema = z.object({
  deviceId: z.string().min(1),
  uptimeSeconds: z.number().int().nonnegative(),
  heapFree: z.number().int().nonnegative(),
  reconnectCount: z.number().int().nonnegative(),
  batteryVoltage: z.number().optional(),
  solarCharging: z.boolean().optional(),
  timestamp: z.string().datetime(),
});

export const mqttCommandAckSchema = z.object({
  commandId: z.string().uuid(),
  success: z.boolean(),
  message: z.string().optional(),
});

export type MqttStatus = z.infer<typeof mqttStatusSchema>;
export type MqttTelemetry = z.infer<typeof mqttTelemetrySchema>;
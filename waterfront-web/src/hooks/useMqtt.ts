// src/hooks/useMqtt.ts
'use client';

import { useEffect, useState, useCallback } from 'react';
import mqtt, { MqttClient, IClientOptions } from 'mqtt';

type ConnectionStatus = 'connected' | 'disconnected' | 'connecting' | 'error';

interface MqttStatusPayload {
  state: string;               // e.g. "returned", "taken", "opened"
  kayakPresent: boolean;
  battery: number;             // %
  connType: 'WiFi' | 'LTE';
  rssi: number;
  // add more fields from your ESP32 publish
}

export function useMqtt() {
  const [client, setClient] = useState<MqttClient | null>(null);
  const [status, setStatus] = useState<ConnectionStatus>('disconnected');
  const [error, setError] = useState<string | null>(null);

  // Example: store latest status per location/machine ID
  const [locationStatuses, setLocationStatuses] = useState<Record<string, MqttStatusPayload>>({});

  const connect = useCallback(() => {
    // ──────────────────────────────────────────────
    // REPLACE WITH YOUR REAL HIVEMQ CLOUD VALUES
    const brokerUrl = 'wss://xxxxxxxx.s1.eu.hivemq.cloud:8884/mqtt'; // ← your cluster wss url
    const options: IClientOptions = {
      clientId: `web-admin-${Math.random().toString(16).slice(3)}`,
      username: 'your-username-from-hivemq-console',
      password: 'your-password-from-hivemq-console',
      clean: true,
      reconnectPeriod: 5000,
      connectTimeout: 30 * 1000,
      protocolVersion: 5, // MQTT 5 recommended
      // rejectUnauthorized: false, // ← only for self-signed cert testing (NOT recommended!)
    };
    // ──────────────────────────────────────────────

    const mqttClient = mqtt.connect(brokerUrl, options);

    mqttClient.on('connect', () => {
      console.log('MQTT Connected to HiveMQ Cloud');
      setStatus('connected');
      setError(null);

      // Subscribe to all location/machine status updates
      // Adjust topic pattern to match your ESP32 publish structure
      mqttClient.subscribe('/kayak/+/status', { qos: 1 }, (err) => {
        if (err) {
          console.error('Subscribe error:', err);
        } else {
          console.log('Subscribed to /kayak/+/status');
        }
      });
    });

    mqttClient.on('message', (topic, message) => {
      try {
        const payload = JSON.parse(message.toString()) as MqttStatusPayload;
        // Extract location/machine ID from topic (e.g. /kayak/location-uuid/status → location-uuid)
        const locationId = topic.split('/')[2]; // adjust index based on your topic

        setLocationStatuses((prev) => ({
          ...prev,
          [locationId]: payload,
        }));
      } catch (e) {
        console.error('Invalid MQTT payload:', e);
      }
    });

    mqttClient.on('error', (err) => {
      console.error('MQTT Error:', err);
      setStatus('error');
      setError(err.message || 'Connection error');
    });

    mqttClient.on('offline', () => {
      setStatus('disconnected');
    });

    mqttClient.on('reconnect', () => {
      setStatus('connecting');
    });

    setClient(mqttClient);

    // Cleanup on unmount / reconnect
    return () => {
      mqttClient.end();
    };
  }, []);

  useEffect(() => {
    connect();

    return () => {
      if (client) {
        client.end();
      }
    };
  }, [connect, client]);

  return {
    status,
    error,
    locationStatuses,
    isConnected: status === 'connected',
    reconnect: connect,
  };
}
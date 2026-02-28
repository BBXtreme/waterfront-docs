// mqtt_topics.h - Central definitions for MQTT topics and QoS settings
// This header defines all MQTT topics used in the Waterfront system.
// Topics are structured hierarchically for location-based booking sync and gate control.
// All status topics use QoS 1 and retained=true for real-time sync.

#ifndef MQTT_TOPICS_H
#define MQTT_TOPICS_H

// Base topic prefix
#define MQTT_BASE_TOPIC "waterfront"

// Hierarchical topic formats (use snprintf for building)
// Format: waterfront/locations/{locationCode}/compartments/{compartmentId}/...
#define MQTT_COMPARTMENT_STATUS_FMT MQTT_BASE_TOPIC "/locations/%s/compartments/%d/status"  // Retained JSON status
#define MQTT_COMPARTMENT_COMMAND_FMT MQTT_BASE_TOPIC "/locations/%s/compartments/%d/command"  // Commands like open_gate, close_gate
#define MQTT_COMPARTMENT_ACK_FMT MQTT_BASE_TOPIC "/locations/%s/compartments/%d/ack"  // Confirmation messages
#define MQTT_COMPARTMENT_EVENT_FMT MQTT_BASE_TOPIC "/locations/%s/compartments/%d/event"  // Taken/returned events

// Wildcard patterns for subscription (match this locationCode)
#define MQTT_COMPARTMENT_STATUS_WILDCARD MQTT_BASE_TOPIC "/locations/%s/compartments/+/status"
#define MQTT_COMPARTMENT_COMMAND_WILDCARD MQTT_BASE_TOPIC "/locations/%s/compartments/+/command"

// QoS settings
#define MQTT_QOS_STATUS 1  // QoS 1 for status (at least once)
#define MQTT_QOS_COMMAND 1  // QoS 1 for commands
#define MQTT_QOS_ACK 1  // QoS 1 for acks
#define MQTT_QOS_EVENT 1  // QoS 1 for events

// Retained flags
#define MQTT_RETAIN_STATUS true  // Status topics are retained
#define MQTT_RETAIN_COMMAND false  // Commands not retained
#define MQTT_RETAIN_ACK false  // Acks not retained
#define MQTT_RETAIN_EVENT false  // Events not retained

#endif // MQTT_TOPICS_H

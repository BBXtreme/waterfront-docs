# WATERFRONT ESP32 Troubleshooting Guide

## Common Issues

### ESP32 Not Starting
- Check power supply
- Verify firmware upload
- Check serial logs for errors

### WiFi Connection Fails
- Verify SSID and password in config
- Check signal strength
- Use provisioning to reconfigure

### MQTT Connection Fails
- Verify broker address and port
- Check TLS settings and certs
- Ensure credentials are correct

### Sensors Not Working
- Check pin assignments in config
- Verify sensor connections
- Check ultrasonic sensor range

### OTA Fails
- Ensure device is connected to network
- Check OTA password
- Monitor progress via MQTT

### Low Power Issues
- Check battery and solar voltage
- Adjust thresholds in config
- Monitor deep sleep cycles

## Logs
- Enable debug mode in config for verbose logs
- Check serial output for error codes
- Monitor MQTT debug topic

## Recovery
- Factory reset: Hold GPIO 0 for 5 seconds
- Re-upload firmware if corrupted
- Restore config.json from backup

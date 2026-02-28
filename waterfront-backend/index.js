const express = require('express');
const bodyParser = require('body-parser');
const { createClient } = require('@supabase/supabase-js');
const mqtt = require('mqtt');
const stripe = require('stripe')(process.env.STRIPE_SECRET_KEY);
const crypto = require('crypto');

// Initialize Express app
const app = express();
app.use(bodyParser.json({
  verify: (req, res, buf) => {
    req.rawBody = buf;
  }
}));

// Initialize Supabase client with env vars
const supabaseUrl = process.env.SUPABASE_URL;
const supabaseKey = process.env.SUPABASE_KEY;
const supabase = createClient(supabaseUrl, supabaseKey);

// MQTT client
const mqttBroker = process.env.MQTT_BROKER || 'mqtt://localhost:1883';
const mqttClient = mqtt.connect(mqttBroker);

mqttClient.on('connect', () => {
  console.log('MQTT connected');
});

mqttClient.on('error', (err) => {
  console.error('MQTT error:', err);
});

// Shared payment success handler
async function handlePaymentSuccess(bookingId, paymentId) {
  try {
    // Query Supabase for booking details
    const { data: booking, error: fetchError } = await supabase
      .from('bookings')
      .select('location_slug, location_code, compartment_number')
      .eq('id', bookingId)
      .single();

    if (fetchError || !booking) {
      console.error('Booking not found:', fetchError);
      return; // Don't throw, just log and skip publishing
    }

    // Update booking status in Supabase
    const { error: updateError } = await supabase
      .from('bookings')
      .update({ status: 'paid', payment_id: paymentId })
      .eq('id', bookingId);

    if (updateError) {
      console.error('Failed to update booking:', updateError);
      throw new Error('Failed to update booking');
    }

    // Publish MQTT unlock message
    const topic = `waterfront/${booking.location_slug}/${booking.location_code}/compartments/${booking.compartment_number}/unlock`;
    const payload = JSON.stringify({ bookingId, durationSec: 7200 });
    mqttClient.publish(topic, payload);

    console.log(`Published MQTT unlock for booking ${bookingId} to ${topic}`);
  } catch (err) {
    console.error('Error in handlePaymentSuccess:', err);
    throw err;
  }
}

// Basic route for testing
app.get('/', (req, res) => {
  res.send('Waterfront Backend is running');
});

// Stripe webhook handler
app.post('/webhook/stripe', async (req, res) => {
  const sig = req.headers['stripe-signature'];
  const endpointSecret = process.env.STRIPE_WEBHOOK_SECRET;

  let event;

  try {
    event = stripe.webhooks.constructEvent(req.rawBody, sig, endpointSecret);
  } catch (err) {
    console.error('Webhook signature verification failed:', err.message);
    return res.status(400).send(`Webhook Error: ${err.message}`);
  }

  // Handle the event
  if (event.type === 'checkout.session.completed') {
    const session = event.data.object;
    const bookingId = session.metadata.bookingId;

    if (!bookingId) {
      console.error('No bookingId in session metadata');
      return res.status(400).send('Missing bookingId');
    }

    try {
      await handlePaymentSuccess(bookingId, session.id);
    } catch (err) {
      return res.status(500).send('Internal server error');
    }
  } else {
    console.log(`Unhandled event type: ${event.type}`);
    return res.status(400).send('Unhandled event type');
  }

  res.json({ received: true });
});

// BTCPay webhook handler
app.post('/webhook/btcpay', async (req, res) => {
  const sig = req.headers['btcpay-sig'];
  const webhookSecret = process.env.BTCPAY_WEBHOOK_SECRET;

  if (!sig || !webhookSecret) {
    console.error('BTCPay webhook: Missing signature or secret');
    return res.status(400).send('Missing signature or secret');
  }

  // Verify signature
  const expectedSig = crypto.createHmac('sha256', webhookSecret).update(req.rawBody).digest('hex');
  if (sig !== expectedSig) {
    console.error('BTCPay webhook signature verification failed');
    return res.status(400).send('Invalid signature');
  }

  // Handle the event
  if (req.body.type === 'InvoicePaid' || req.body.type === 'InvoiceSettled') {
    const invoice = req.body.invoice;
    const bookingId = invoice.metadata.bookingId;

    if (!bookingId) {
      console.error('No bookingId in invoice metadata');
      return res.status(400).send('Missing bookingId');
    }

    try {
      await handlePaymentSuccess(bookingId, invoice.id);
    } catch (err) {
      return res.status(500).send('Internal server error');
    }
  } else {
    console.log(`Unhandled BTCPay event type: ${req.body.type}`);
    return res.status(400).send('Unhandled event type');
  }

  res.status(200).json({ received: true });
});

// Start server
const port = process.env.PORT || 3000;
app.listen(port, () => {
  console.log(`Backend on ${port}`);
});

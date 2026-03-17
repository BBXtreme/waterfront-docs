# Admin Dashboard Layout Specification

## Overview

- Purpose: Operator/admin interface for monitoring and managing rentals
- Target users: Business owner / staff (authenticated)
- Not a PWA: standard browser application, no install prompt, no offline support
- Access: Protected routes under `/admin/*`
- Authentication: Supabase email/password + role check (admin role)
- Theme: Dark mode optional, professional (gray/blue tones, tables, charts)
- Layout: Sidebar navigation + top bar (user info, logout)

## Global Admin Layout

- Sidebar (fixed left, collapsible on mobile)
  - Dashboard
  - Bookings
  - Machines / Telemetry
  - Payments & Deposits
  - Logs / Events
  - Settings (future)
- Top bar: Logo, current user email, logout button
- Main content: Responsive container (max-w-7xl)

## Screens

### 1. Login (/admin/login)

- Centered card: email + password
- "Forgot password?" link
- Error messages (invalid credentials)
- Redirect to /admin/dashboard on success

### 2. Dashboard Overview (/admin/dashboard)

- Grid of stat cards:
  - Active rentals (count)
  - Available machines (count)
  - Revenue today (EUR + BTC)
  - Alerts (low battery, overdue returns)
- Quick links to bookings/machines

### 3. Bookings List (/admin/bookings)

- DataTable (shadcn) with columns:
  - ID
  - Customer email/phone (if collected)
  - Machine / Location
  - Start – End time
  - Status (pending/active/completed/cancelled/overdue)
  - PIN
  - Payment status (paid/pending/refunded)
  - Actions: View details, manual refund, send reminder
- Filters: date range, status, location
- Pagination + search

### 4. Machines / Telemetry (/admin/machines)

- Realtime table (Supabase subscription):
  - Machine ID
  - Location
  - Battery %
  - Status (idle/rented/offline)
  - Last event (timestamp)
  - Connection (WiFi/LTE, RSSI)
  - Actions: Manual unlock/lock (MQTT publish)
- Status badges (green/red/yellow)
- Low battery threshold alert

### 5. Logs / Events (/admin/logs)

- Table of MQTT events + system logs:
  - Timestamp
  - Machine ID
  - Event type (taken/returned/unlock/offline)
  - Payload (JSON preview)
- Search + filter by machine/time

## Security & Access Control

- All /admin/* routes protected
- Use Supabase auth + row-level security (RLS)
- Admin role check in middleware or layout
- No public API exposure for admin actions

## Future Extensions

- Damage reports with photo upload/view
- Bulk actions (cancel multiple bookings)
- Export CSV reports
- User management (add/remove operators)

## References

- RentalBuddy admin screenshots (if available)
- Supabase dashboard style inspiration
-- Migration: Extend Supabase schema for Waterfront multi-location, multi-box, multi-compartment support
-- This migration adds hierarchical tables for locations, boxes, and compartments.
-- Updates bookings to reference compartments instead of old slot/machine fields.
-- Run with: supabase db reset (for local dev) or apply via Supabase dashboard.

-- Enable UUID extension if not already
CREATE EXTENSION IF NOT EXISTS "uuid-ossp";

-- Table: locations
-- Represents physical locations (e.g., "Bremen Harbor", "Lake Wannsee")
CREATE TABLE locations (
    id UUID PRIMARY KEY DEFAULT uuid_generate_v4(),
    name TEXT NOT NULL,
    slug TEXT UNIQUE NOT NULL,  -- e.g., "bremen-harbor" for URL-friendly identifiers
    created_at TIMESTAMP WITH TIME ZONE DEFAULT NOW(),
    updated_at TIMESTAMP WITH TIME ZONE DEFAULT NOW()
);

-- Table: location_boxes
-- Represents individual vending machines/boxes at a location (e.g., "Box 1" at Bremen Harbor)
CREATE TABLE location_boxes (
    id UUID PRIMARY KEY DEFAULT uuid_generate_v4(),
    location_id UUID NOT NULL REFERENCES locations(id) ON DELETE CASCADE,
    code TEXT UNIQUE NOT NULL,  -- e.g., "bremen-harbor-01" for MQTT/machine ID
    compartments_count INTEGER NOT NULL DEFAULT 1 CHECK (compartments_count > 0),
    created_at TIMESTAMP WITH TIME ZONE DEFAULT NOW(),
    updated_at TIMESTAMP WITH TIME ZONE DEFAULT NOW()
);

-- Table: compartments
-- Represents individual storage compartments within a box (e.g., slot 1, 2, 3 in Box 1)
CREATE TABLE compartments (
    id UUID PRIMARY KEY DEFAULT uuid_generate_v4(),
    location_box_id UUID NOT NULL REFERENCES location_boxes(id) ON DELETE CASCADE,
    number INTEGER NOT NULL CHECK (number > 0),  -- Compartment number (1, 2, 3, etc.)
    equipment_id UUID,  -- Optional FK to equipment table (if tracking specific kayaks/gear)
    created_at TIMESTAMP WITH TIME ZONE DEFAULT NOW(),
    updated_at TIMESTAMP WITH TIME ZONE DEFAULT NOW(),
    UNIQUE(location_box_id, number)  -- Ensure unique compartment numbers per box
);

-- Update bookings table: Add compartment_id FK, remove old slot/machine refs
-- Assumes bookings table exists from previous migration; adjust if needed
ALTER TABLE bookings
ADD COLUMN compartment_id UUID REFERENCES compartments(id) ON DELETE SET NULL;

-- Optional: Drop old columns if they exist (uncomment if applicable)
-- ALTER TABLE bookings DROP COLUMN IF EXISTS slot_id;
-- ALTER TABLE bookings DROP COLUMN IF EXISTS machine_id;

-- Indexes for performance
CREATE INDEX idx_location_boxes_location_id ON location_boxes(location_id);
CREATE INDEX idx_compartments_location_box_id ON compartments(location_box_id);
CREATE INDEX idx_bookings_compartment_id ON bookings(compartment_id);

-- Row Level Security (RLS) policies (enable if using Supabase auth)
-- Example: Allow authenticated users to read locations
ALTER TABLE locations ENABLE ROW LEVEL SECURITY;
CREATE POLICY "Locations are viewable by authenticated users" ON locations
FOR SELECT USING (auth.uid() IS NOT NULL);

-- Similarly for other tables (add as needed for your app's auth model)

-- Insert sample data for testing (optional, remove in production)
INSERT INTO locations (name, slug) VALUES
('Bremen Harbor', 'bremen-harbor'),
('Lake Wannsee', 'lake-wannsee');

INSERT INTO location_boxes (location_id, code, compartments_count) VALUES
((SELECT id FROM locations WHERE slug = 'bremen-harbor'), 'bremen-harbor-01', 3),
((SELECT id FROM locations WHERE slug = 'lake-wannsee'), 'lake-wannsee-01', 2);

INSERT INTO compartments (location_box_id, number) VALUES
((SELECT id FROM location_boxes WHERE code = 'bremen-harbor-01'), 1),
((SELECT id FROM location_boxes WHERE code = 'bremen-harbor-01'), 2),
((SELECT id FROM location_boxes WHERE code = 'bremen-harbor-01'), 3),
((SELECT id FROM location_boxes WHERE code = 'lake-wannsee-01'), 1),
((SELECT id FROM location_boxes WHERE code = 'lake-wannsee-01'), 2);

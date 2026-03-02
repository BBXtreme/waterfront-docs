-- Create the bookings table in Supabase
CREATE TABLE bookings (
  id UUID PRIMARY KEY DEFAULT gen_random_uuid(),
  name TEXT NOT NULL,
  email TEXT NOT NULL,
  date TEXT NOT NULL,
  time TEXT NOT NULL,
  kayak_type TEXT NOT NULL,
  created_at TIMESTAMPTZ DEFAULT NOW()
);

-- Enable Row Level Security
ALTER TABLE bookings ENABLE ROW LEVEL SECURITY;

-- Create a policy to allow anonymous inserts (for demo purposes)
-- In production, you might want to restrict this or use authentication
CREATE POLICY "Allow anonymous inserts" ON bookings
FOR INSERT
WITH CHECK (true);

-- Optional: Create a policy to allow reading bookings (if needed)
CREATE POLICY "Allow anonymous reads" ON bookings
FOR SELECT
USING (true);

'use client';

import { createClient } from '@supabase/supabase-js';

export default function SupabaseTest() {
  const supabase = createClient(
    process.env.NEXT_PUBLIC_SUPABASE_URL!,
    process.env.NEXT_PUBLIC_SUPABASE_ANON_KEY!
  );

  // Optional: simple status check (uncomment later when you have a table)
  // const { data, error } = await supabase.from('test_table').select('*').limit(1);

  return (
    <div className="min-h-screen flex flex-col items-center justify-center p-8 bg-gray-50">
      <h1 className="text-3xl font-bold mb-6">Local Supabase Connection Test</h1>
      
      <div className="bg-white p-6 rounded-lg shadow-md max-w-md w-full">
        <p className="mb-4">
          <strong>URL:</strong> {process.env.NEXT_PUBLIC_SUPABASE_URL || 'Not set'}
        </p>
        <p className="mb-4">
          <strong>Anon Key length:</strong> {process.env.NEXT_PUBLIC_SUPABASE_ANON_KEY?.length || 0}
          {' '}({process.env.NEXT_PUBLIC_SUPABASE_ANON_KEY ? '✅ Loaded' : '❌ Missing'})
        </p>
        
        <p className="text-sm text-gray-600 mt-4">
          If key length is ~300–400 → connection ready.  
          Next: add auth or query a table.
        </p>
      </div>
    </div>
  );
}
'use client';

import { useTheme } from 'next-themes';
import { Toggle } from '@/components/ui/toggle';
import { Moon, Sun } from 'lucide-react';
import { useEffect, useState } from 'react';
import { cn } from '@/lib/utils'; // ← THIS WAS MISSING – add this line!

export function ThemeToggle() {
  const { theme, setTheme } = useTheme();
  const [mounted, setMounted] = useState(false);

  useEffect(() => {
    setMounted(true);
  }, []);

  if (!mounted) {
    return <div className="h-10 w-10" aria-hidden="true" />;
  }

  const isDark = theme === 'dark';

  return (
    <Toggle
      pressed={isDark}
      onPressedChange={() => setTheme(isDark ? 'light' : 'dark')}
      aria-label={`Switch to ${isDark ? 'light' : 'dark'} mode`}
      className={cn(
        'data-[state=on]:bg-amber-500 data-[state=on]:text-amber-950',
        'data-[state=off]:bg-slate-200 data-[state=off]:text-slate-900',
        'hover:bg-gray-200/80 dark:hover:bg-gray-800/80',
        'transition-colors duration-200'
      )}
    >
      {isDark ? <Sun className="h-5 w-5" /> : <Moon className="h-5 w-5" />}
    </Toggle>
  );
}
import { ReactNode } from 'react';
import { Card, CardContent, CardHeader, CardTitle } from '@/components/ui/card';
import { Badge } from '@/components/ui/badge';
import { cn } from '@/lib/utils';

interface MachineCardProps {
  title: string;
  status: string;
  message: string;
  timestamp?: string;
  isConnected?: boolean;
  children?: ReactNode;
}

export default function MachineCard({
  title,
  status,
  message,
  timestamp,
  isConnected = false,
  children,
}: MachineCardProps) {
  const isPositive = status.includes("OK") || status.includes("Connected") || status.includes("Detected") || status.includes("connected");
  return (
    <Card className="shadow-sm rounded-lg overflow-hidden w-full bg-card border border-border">
      <CardHeader className="p-[25px]">
        <div className="flex justify-between items-center">
          <CardTitle className="font-medium">{title}</CardTitle>
          <Badge
            variant="outline"
            className={cn(
              "px-2 py-1 rounded-full text-xs font-medium",
              isPositive ? "bg-green-50 text-green-700 dark:bg-green-950 dark:text-green-300" : "bg-red-50 text-red-700 dark:bg-red-950 dark:text-red-300"
            )}
          >
            {status}
          </Badge>
        </div>
      </CardHeader>
      <CardContent className="p-[25px] flex flex-col gap-4 text-sm">
        <div>
          <p className="text-muted-foreground">{message}</p>
          {timestamp && <p className="text-xs text-muted-foreground">Last checked: {timestamp}</p>}
        </div>
        {children}
      </CardContent>
    </Card>
  );
}

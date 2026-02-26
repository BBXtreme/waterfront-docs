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
  return (
    <Card className="m-4 shadow-sm rounded-lg overflow-hidden w-full bg-gradient-to-br from-card to-card/80">
      <CardHeader className="p-6">
        <div className="flex justify-between items-center">
          <CardTitle className="font-medium">{title}</CardTitle>
          <Badge
            variant="default"
            className={cn(
              isConnected
                ? 'bg-green-50 text-green-700 dark:bg-green-950 dark:text-green-300'
                : ''
            )}
          >
            {status}
          </Badge>
        </div>
      </CardHeader>
      <CardContent className="p-6 flex flex-col gap-4 text-sm">
        <div>
          <p className="text-muted-foreground">{message}</p>
          {timestamp && <p className="text-xs text-muted-foreground">Last checked: {timestamp}</p>}
        </div>
        {children}
      </CardContent>
    </Card>
  );
}

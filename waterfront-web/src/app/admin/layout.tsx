// src/app/admin/layout.tsx
'use client';

import type { ReactNode } from 'react';
import { usePathname } from 'next/navigation';
import Link from 'next/link';
import {
  Sidebar,
  SidebarContent,
  SidebarFooter,
  SidebarGroup,
  SidebarGroupContent,
  SidebarGroupLabel,
  SidebarHeader,
  SidebarMenu,
  SidebarMenuButton,
  SidebarMenuItem,
  SidebarMenuSub,
  SidebarMenuSubButton,
  SidebarMenuSubItem,
  SidebarProvider,
  SidebarTrigger,
  useSidebar,
} from '@/components/ui/sidebar';
import {
  ChevronDown,
  ChevronRight,
  LayoutDashboard,
  CalendarCheck,
  MapPin,
  PlugZap,
  FileText,
  User,
  LogOut,
  Wifi,
  WifiOff,
} from 'lucide-react';
import { cn } from '@/lib/utils';
import { Tooltip, TooltipContent, TooltipProvider, TooltipTrigger } from '@/components/ui/tooltip';

// Navigation structure
const navMain = [
  {
    title: 'Dashboard',
    url: '/admin/dashboard',
    icon: LayoutDashboard,
  },
  {
    title: 'Bookings',
    url: '/admin/bookings',
    icon: CalendarCheck,
  },
  {
    title: 'Locations',
    icon: MapPin,
    isCollapsible: true,
    items: [
      { title: 'All Locations', url: '/admin/locations' },
      { title: 'Add New Location', url: '/admin/locations/new' },
    ],
  },
  {
    title: 'Connections',
    url: '/admin/connections',
    icon: PlugZap,
  },
  {
    title: 'Logs',
    url: '/admin/logs',
    icon: FileText,
    accent: true,
  },
];

// Mock MQTT status (replace with real data later – from context, Zustand, Supabase realtime, etc.)
const mockConnectionStatus = {
  connected: true,
  type: 'WiFi', // or 'LTE'
  rssi: -58,
  lastSeen: '2 min ago',
};

export default function AdminLayout({ children }: { children: ReactNode }) {
  const pathname = usePathname();

  return (
    <TooltipProvider>
      <SidebarProvider defaultOpen={true}>
        <div className="flex min-h-screen bg-background">
          {/* Sidebar */}
          <Sidebar collapsible="icon" className="border-r border-border">
            <SidebarHeader className="border-b border-border">
              <div className="flex items-center gap-2 px-4 py-3">
                <MapPin className="h-6 w-6 text-waterfront-primary" />
                <span className="font-semibold text-lg tracking-tight text-foreground">
                  Waterfront Admin
                </span>
              </div>
            </SidebarHeader>

            <SidebarContent>
              <SidebarGroup>
                <SidebarGroupContent>
                  <SidebarMenu>
                    {navMain.map((item) => {
                      const isActive = pathname === item.url || 
                        (item.items && item.items.some(sub => pathname === sub.url));

                      if (item.isCollapsible) {
                        return (
                          <CollapsibleLocationItem
                            key={item.title}
                            item={item}
                            isActive={isActive}
                            pathname={pathname}
                          />
                        );
                      }

                      return (
                        <SidebarMenuItem key={item.url}>
                          <SidebarMenuButton
                            asChild
                            tooltip={item.title}
                            isActive={isActive}
                            className={cn(
                              'transition-colors',
                              item.accent &&
                                'text-btc-accent hover:bg-btc-accent/10 data-[active=true]:bg-btc-accent/15 data-[active=true]:text-btc-accent',
                              isActive && !item.accent && 'bg-accent text-accent-foreground'
                            )}
                          >
                            <Link href={item.url!}>
                              <item.icon className="h-5 w-5" />
                              <span>{item.title}</span>
                            </Link>
                          </SidebarMenuButton>
                        </SidebarMenuItem>
                      );
                    })}
                  </SidebarMenu>
                </SidebarGroupContent>
              </SidebarGroup>
            </SidebarContent>

            {/* Footer – User profile / logout */}
            <SidebarFooter className="border-t border-border p-4">
              <SidebarMenu>
                <SidebarMenuItem>
                  <SidebarMenuButton asChild tooltip="Profile">
                    <div className="flex items-center gap-3">
                      <div className="h-8 w-8 rounded-full bg-muted flex items-center justify-center">
                        <User className="h-4 w-4" />
                      </div>
                      <div className="flex flex-col text-left text-sm">
                        <span className="font-medium">Admin User</span>
                        <span className="text-xs text-muted-foreground">admin@waterfront.de</span>
                      </div>
                    </div>
                  </SidebarMenuButton>
                </SidebarMenuItem>

                <SidebarMenuItem>
                  <SidebarMenuButton tooltip="Log out" className="text-destructive hover:text-destructive/90">
                    <LogOut className="h-5 w-5" />
                    <span>Log out</span>
                  </SidebarMenuButton>
                </SidebarMenuItem>
              </SidebarMenu>
            </SidebarFooter>
          </Sidebar>

          {/* Main content */}
          <div className="flex-1 flex flex-col">
            <header className="sticky top-0 z-30 flex h-14 items-center gap-4 border-b bg-background/95 backdrop-blur-sm px-6">
              <SidebarTrigger className="md:hidden" />
              <div className="hidden md:flex items-center gap-2">
                <SidebarTrigger />
              </div>

              {/* MQTT connection status indicator */}
              <div className="ml-auto flex items-center gap-4">
                <Tooltip>
                  <TooltipTrigger asChild>
                    <div className="flex items-center gap-2 text-sm">
                      <div
                        className={cn(
                          'h-3 w-3 rounded-full animate-pulse',
                          mockConnectionStatus.connected ? 'bg-green-500' : 'bg-red-500'
                        )}
                      />
                      <span className="hidden sm:inline font-medium">
                        {mockConnectionStatus.connected ? 'Connected' : 'Disconnected'}
                      </span>
                      <span className="text-xs text-muted-foreground">
                        {mockConnectionStatus.type}
                      </span>
                    </div>
                  </TooltipTrigger>
                  <TooltipContent side="bottom" className="text-xs">
                    <p>MQTT Broker Status</p>
                    <p className="text-muted-foreground">
                      {mockConnectionStatus.connected
                        ? `Connected via ${mockConnectionStatus.type} • RSSI: ${mockConnectionStatus.rssi} dBm`
                        : 'No connection • Last seen: never'}
                    </p>
                    <p className="text-xs text-muted-foreground mt-1">
                      Last update: {mockConnectionStatus.lastSeen}
                    </p>
                  </TooltipContent>
                </Tooltip>
              </div>
            </header>

            <main className="flex-1 overflow-auto p-6 md:p-8">{children}</main>
          </div>
        </div>
      </SidebarProvider>
    </TooltipProvider>
  );
}

// Helper component for collapsible Locations submenu
function CollapsibleLocationItem({
  item,
  isActive,
  pathname,
}: {
  item: any;
  isActive: boolean;
  pathname: string;
}) {
  const { state } = useSidebar();

  return (
    <SidebarMenuItem>
      <SidebarMenuButton
        tooltip={item.title}
        className={cn(
          'justify-between',
          isActive && 'bg-accent text-accent-foreground'
        )}
      >
        <div className="flex items-center gap-3">
          <item.icon className="h-5 w-5" />
          <span>{item.title}</span>
        </div>
        {state === 'expanded' && (
          <ChevronDown className="h-4 w-4 transition-transform data-[state=open]:rotate-180" />
        )}
      </SidebarMenuButton>

      {state === 'expanded' && (
        <SidebarMenuSub>
          {item.items.map((subItem: any) => (
            <SidebarMenuSubItem key={subItem.url}>
              <SidebarMenuSubButton
                asChild
                isActive={pathname === subItem.url}
              >
                <Link href={subItem.url}>
                  <span>{subItem.title}</span>
                </Link>
              </SidebarMenuSubButton>
            </SidebarMenuSubItem>
          ))}
        </SidebarMenuSub>
      )}
    </SidebarMenuItem>
  );
}
import { Button, ButtonProps } from '@/components/ui/button';
import { cn } from '@/lib/utils';

const buttonVariants = {
  default: `
    w-full max-w-xs
    bg-gray-200 hover:bg-white active:bg-gray-50
    text-gray-900 border border-gray-300
    rounded-full px-10 py-3.5
    text-md font-normal
    shadow-sm hover:shadow
    transition-all duration-250 ease-out
    focus:outline-none focus:ring-2 focus:ring-gray-400 focus:ring-offset-2
  `,
  // You can add more variants later: primary, outline, destructive, etc.
};

type CustomButtonProps = ButtonProps & {
  variant?: keyof typeof buttonVariants;
};

export function CustomButton({
  className,
  variant = 'default',
  ...props
}: CustomButtonProps) {
  return (
    <Button
      className={cn(buttonVariants[variant], className)}
      {...props}
    />
  );
}
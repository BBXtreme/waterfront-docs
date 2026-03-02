# Waterfront Style Guide

**Version**: 1.0  
**Date**: March 2026  
**Purpose**: Enforce consistent, professional, and thematic UI across the PWA booking flow, admin dashboard, and ESP32-related telemetry views.  
Blend: x.ai minimalism (clean cards, whitespace, subtle elegance) + thebitcoinportal.com energy (vibrant orange accents, gradients for action/BTC) + waterfront personality (fluid blues, subtle wave motifs).

## 1. Core Principles
- **Minimal yet Energetic**: Clean layouts with generous whitespace, but punchy accents for CTAs and BTC-related flows.
- **Mobile-First & PWA-Optimized**: Responsive, touch-friendly, high contrast for outdoor use.
- **Theming**: Full light/dark mode support via Tailwind `dark:` and CSS variables.
- **Accessibility**: Minimum 4.5:1 contrast, focus states, reduced motion where possible.
- **Performance**: Use Tailwind utilities; avoid heavy custom CSS.

## 2. Color Palette

Use CSS variables defined in `globals.css` / extended in `tailwind.config.js`.

### Primary Colors
- `--waterfront-primary`: #0EA5E9   (bright cyan-blue – main actions, headers)
- `--waterfront-primary-dark`: #0284C7 (darker hover variant)
- `--btc-accent`: #F7931A          (Bitcoin orange – payments, success states, BTC buttons)
- `--btc-accent-dark`: #D97706     (darker BTC hover)

### Neutrals & Backgrounds
- Light mode: `--background`: #FFFFFF / `--foreground`: #111827
- Dark mode:  `--background`: #0F172A / `--foreground`: #F1F5F9
- `--muted`: #6B7280 (secondary text)
- `--border`: #E2E8F0 (light) / #334155 (dark)

### Semantic Colors
- Success: #10B981 (green-500)
- Warning: #F59E0B (amber-500)
- Destructive: #EF4444 (red-500)
- Info: #3B82F6 (blue-500)

### Gradients (for energy / BTC flows)
- BTC gradient: `linear-gradient(135deg, #F7931A 0%, #F5A623 100%)`
- Waterfront wave gradient: `linear-gradient(to right, #0EA5E9 0%, #0284C7 100%)`
- Subtle card overlay: `linear-gradient(to bottom, rgba(255,255,255,0.05), rgba(255,255,255,0))` (light mode)

## 3. Typography

Font stack (add to `globals.css` or via `@import`):
```css
@import url('https://fonts.googleapis.com/css2?family=Inter:wght@300;400;500;600;700&family=Space+Grotesk:wght@500;600;700&display=swap');
```

- **Headings**:
  - H1: font-space-grotesk font-bold text-4xl md:text-5xl tracking-tight
  - H2: font-space-grotesk font-semibold text-3xl md:text-4xl
  - H3: font-inter font-semibold text-2xl
- **Body**:
  - Default: text-base leading-relaxed text-foreground
  - Small/Muted: text-sm text-muted-foreground
- **Buttons / CTAs**:
  - font-medium text-base
- **Mono/Numbers** (for prices, PINs, telemetry):
  - font-space-grotesk font-medium

## 4. Spacing & Layout

- **Container**: Use Tailwind container (center + padding-2rem, max 1400px)
- **Spacing scale**: 4px base → p-4, gap-6, m-8, etc.
- **Card padding**: p-6 or p-8 for larger admin cards
- **Section spacing**: py-12 md:py-20
- **Whitespace**: Generous – avoid crowding; use space-y-8 / gap-10

## 5. Components

### Buttons

tsx

```
<Button 
  className="bg-waterfront-primary hover:bg-waterfront-primary-dark text-white font-medium rounded-lg px-6 py-3 transition-all shadow-sm hover:shadow-md"
  // or for BTC:
  className="bg-gradient-to-r from-btc-accent to-[#F5A623] hover:brightness-110 text-white font-medium rounded-lg px-6 py-3 shadow-md hover:shadow-lg"
/>
```

- Variants: default (primary blue), btc (orange gradient), outline, ghost, destructive
- Sizes: sm (px-4 py-2), default (px-6 py-3), lg (px-8 py-4)
- Hover: brightness-110 or scale-105 (subtle)

### Cards

tsx

```
<Card className="border border-border bg-card shadow-sm rounded-xl overflow-hidden">
  <CardHeader className="pb-4">
    <CardTitle className="text-xl font-semibold">Booking #1234</CardTitle>
  </CardHeader>
  <CardContent className="p-6 pt-0">
    {/* content */}
  </CardContent>
</Card>
```

- Style: Minimal borders, subtle shadow (shadow-sm → shadow-md on hover), rounded-xl
- Admin variant: Add dark:bg-slate-800/50 backdrop-blur-sm for glass effect

### Badges / Status

- Available: bg-green-100 text-green-800 dark:bg-green-900/30 dark:text-green-400
- Booked: bg-amber-100 text-amber-800 dark:bg-amber-900/30
- BTC Payment: bg-gradient-to-r from-btc-accent/20 to-btc-accent/10 text-btc-accent border-btc-accent/30

### Inputs / Forms

- Use shadcn/ui Input: border-input focus:ring-waterfront-primary focus:border-waterfront-primary
- Labels: text-sm font-medium

### Backgrounds (Waterfront twist)

Subtle wave pattern (add to body or hero sections):

CSS

```
.bg-wave {
  background: linear-gradient(135deg, #0f172a 0%, #1e293b 100%),
              radial-gradient(circle at 20% 30%, rgba(14,165,233,0.08) 0%, transparent 50%);
  background-blend-mode: overlay;
}
```

Or use SVG wave for more fluid effect (inline or component).

## 6. Dark Mode Adjustments

- Prefer darker blues/oranges for accents in dark mode
- Cards: Slight transparency or deeper shadows
- Text: Higher contrast (e.g. --foreground #F1F5F9)

## 7. Usage Guidelines for AI Coding (Aider / Grok / Claude)

- Always use semantic Tailwind classes + variables (e.g. bg-waterfront-primary not #0EA5E9)
- Prefer shadcn/ui components as base
- For payment/BTC screens: Use --btc-accent gradient prominently
- For booking/admin: Lean minimal – clean cards, blue primary
- Add subtle hover transitions (150–200ms)
- Test mobile: Ensure touch targets ≥44×44px

## 8. To-Do / Future Extensions

- Add wave SVG hero backgrounds
- Implement glassmorphism for admin overlays (backdrop-blur)
- Create custom Tailwind variants: btn-btc, card-wave

Happy coding – keep it clean, energetic, and waterfront-fresh!


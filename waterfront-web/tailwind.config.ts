import type { Config } from 'tailwindcss'

export default {
  content: [
    './src/app/**/*.{js,ts,jsx,tsx,mdx}',
    './src/components/**/*.{js,ts,jsx,tsx,mdx}',
  ],
  theme: {
    extend: {
      colors: {
        waterfront: {
          primary: '#0EA5E9',
          'primary-dark': '#0284C7',
        },
        btc: {
          accent: '#F7931A',
          'accent-dark': '#D97706',
        },
      },
    },
  },
  plugins: [],  // no daisyui here anymore
} satisfies Config
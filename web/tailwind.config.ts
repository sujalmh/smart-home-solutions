import type { Config } from "tailwindcss";

const config: Config = {
  content: [
    "./pages/**/*.{js,ts,jsx,tsx,mdx}",
    "./components/**/*.{js,ts,jsx,tsx,mdx}",
    "./app/**/*.{js,ts,jsx,tsx,mdx}",
  ],
  theme: {
    extend: {
      colors: {
        background: "#0b1326",
        primary: "#00daf3",
        "primary-container": "#009fb2",
        "on-primary": "#001f27",
        surface: "#0d1528",
        "surface-container-lowest": "#0d1528",
        "surface-container-low": "#111c35",
        "surface-container": "#1a2540",
        "surface-container-high": "#202d4a",
        "surface-container-highest": "#2a3a5c",
        "on-surface": "#dae2fd",
        "on-surface-variant": "#c2cbe7",
        tertiary: "#7bd1fa",
        "tertiary-container": "#4ab8e8",
        error: "#ffb4ab",
        "outline-variant": "#414754",
        outline: "#8b95b0",
      },
      fontFamily: {
        display: ["var(--font-space-grotesk)", "Space Grotesk", "sans-serif"],
        body: ["var(--font-inter)", "Inter", "sans-serif"],
      },
      fontSize: {
        "display-lg": ["3.5rem", { lineHeight: "1.1", fontWeight: "700" }],
        "display-md": ["2.5rem", { lineHeight: "1.15", fontWeight: "700" }],
        "display-sm": ["2rem", { lineHeight: "1.2", fontWeight: "700" }],
        "title-lg": ["1.5rem", { lineHeight: "1.3", fontWeight: "600" }],
        "title-md": ["1.125rem", { lineHeight: "1.4", fontWeight: "600" }],
        "title-sm": ["0.9375rem", { lineHeight: "1.4", fontWeight: "600" }],
        "body-lg": ["1rem", { lineHeight: "1.6", fontWeight: "400" }],
        "body-md": ["0.875rem", { lineHeight: "1.6", fontWeight: "400" }],
        "body-sm": ["0.75rem", { lineHeight: "1.5", fontWeight: "400" }],
        "label-lg": ["0.875rem", { lineHeight: "1.4", fontWeight: "500" }],
        "label-md": ["0.75rem", { lineHeight: "1.4", fontWeight: "500" }],
        "label-sm": ["0.6875rem", { lineHeight: "1.4", fontWeight: "500" }],
      },
      spacing: {
        "section": "5rem",
        "section-lg": "8rem",
      },
      borderRadius: {
        sm: "0.25rem",
        DEFAULT: "0.5rem",
        md: "0.75rem",
        lg: "1rem",
        xl: "1.5rem",
        full: "9999px",
      },
      boxShadow: {
        float: "0 20px 40px 0 rgba(0,0,0,0.08)",
        "float-lg": "0 30px 60px 0 rgba(0,0,0,0.12)",
      },
      backdropBlur: {
        glass: "20px",
      },
      transitionTimingFunction: {
        precision: "cubic-bezier(0.4, 0, 0.2, 1)",
      },
      transitionDuration: {
        precision: "200ms",
      },
      animation: {
        "pulse-slow": "pulse 4s cubic-bezier(0.4, 0, 0.6, 1) infinite",
      },
    },
  },
  plugins: [],
};

export default config;

import type { Metadata } from "next";
import { Inter, Space_Grotesk } from "next/font/google";
import "./globals.css";

const inter = Inter({
  subsets: ["latin"],
  variable: "--font-inter",
  display: "swap",
});

const spaceGrotesk = Space_Grotesk({
  subsets: ["latin"],
  variable: "--font-space-grotesk",
  display: "swap",
});

export const metadata: Metadata = {
  title: "Smart Home Solutions — Intelligent Automation Platform",
  description:
    "Control every room, every device — intelligently. Real-time smart home automation powered by FastAPI, Flutter, ESP32, and an AI assistant built on LangGraph.",
  keywords: [
    "smart home",
    "home automation",
    "IoT",
    "ESP32",
    "Flutter",
    "FastAPI",
    "AI assistant",
  ],
  openGraph: {
    title: "Smart Home Solutions",
    description: "Control every room, every device — intelligently.",
    type: "website",
  },
};

export default function RootLayout({
  children,
}: Readonly<{
  children: React.ReactNode;
}>) {
  return (
    <html lang="en" className={`${inter.variable} ${spaceGrotesk.variable}`}>
      <body>{children}</body>
    </html>
  );
}

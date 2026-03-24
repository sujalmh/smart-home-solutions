"use client";

import { useEffect, useState } from "react";
import { motion, useScroll, useMotionValueEvent } from "framer-motion";

const navLinks = [
  { label: "Features", href: "#features" },
  { label: "How It Works", href: "#how-it-works" },
  { label: "AI Assistant", href: "#ai" },
  { label: "Tech Stack", href: "#tech" },
];

export default function Nav() {
  const { scrollY } = useScroll();
  const [scrolled, setScrolled] = useState(false);

  useMotionValueEvent(scrollY, "change", (latest) => {
    setScrolled(latest > 40);
  });

  return (
    <motion.header
      className="fixed top-0 left-0 right-0 z-50 transition-all duration-[200ms]"
      style={{
        background: scrolled
          ? "rgba(17, 28, 53, 0.92)"
          : "rgba(11, 19, 38, 0)",
        backdropFilter: scrolled ? "blur(20px)" : "blur(0px)",
        WebkitBackdropFilter: scrolled ? "blur(20px)" : "blur(0px)",
      }}
    >
      <div className="max-w-7xl mx-auto px-6 lg:px-10 flex items-center justify-between h-16">
        {/* Logo */}
        <a href="#" className="flex items-center gap-2.5 group">
          <span className="flex items-center justify-center w-8 h-8 rounded-sm bg-primary/10">
            <HomeSVG />
          </span>
          <span className="font-display font-semibold text-title-sm text-on-surface tracking-tight">
            Smart Home{" "}
            <span className="text-primary">Solutions</span>
          </span>
        </a>

        {/* Nav links — desktop */}
        <nav className="hidden md:flex items-center gap-8">
          {navLinks.map((link) => (
            <a
              key={link.href}
              href={link.href}
              className="text-label-lg font-body text-on-surface-variant hover:text-on-surface transition-colors duration-[200ms]"
            >
              {link.label}
            </a>
          ))}
        </nav>

        {/* CTA */}
        <a
          href="#features"
          className="hidden md:inline-flex items-center gap-2 gradient-cta font-body font-medium text-label-lg px-5 py-2 rounded-full"
        >
          Get Started
        </a>
      </div>
    </motion.header>
  );
}

function HomeSVG() {
  return (
    <svg width="18" height="18" viewBox="0 0 18 18" fill="none" xmlns="http://www.w3.org/2000/svg">
      <path
        d="M9 2L2 8v8h5v-4h4v4h5V8L9 2Z"
        stroke="#00daf3"
        strokeWidth="1.5"
        strokeLinejoin="round"
        fill="none"
      />
    </svg>
  );
}

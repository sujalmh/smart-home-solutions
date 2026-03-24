"use client";

import { motion } from "framer-motion";

const ease = [0.4, 0, 0.2, 1] as const;

export default function CTABanner() {
  return (
    <section
      className="relative py-section-lg overflow-hidden"
      style={{
        background: "linear-gradient(135deg, #00daf3 0%, #009fb2 100%)",
      }}
    >
      {/* Subtle pattern overlay */}
      <div
        aria-hidden
        className="pointer-events-none absolute inset-0 opacity-[0.06]"
        style={{
          backgroundImage: `repeating-linear-gradient(
            45deg,
            #001f27 0px,
            #001f27 1px,
            transparent 1px,
            transparent 20px
          )`,
        }}
      />

      {/* Ambient glow top-right */}
      <div
        aria-hidden
        className="pointer-events-none absolute -top-16 right-[-10%] w-[40vw] h-[40vh] rounded-full opacity-20"
        style={{
          background: "radial-gradient(circle, #7bd1fa 0%, transparent 70%)",
          filter: "blur(60px)",
        }}
      />

      <div className="relative z-10 max-w-4xl mx-auto px-6 lg:px-10 flex flex-col items-center text-center gap-8">
        <motion.div
          initial={{ opacity: 0, y: 24 }}
          whileInView={{ opacity: 1, y: 0 }}
          viewport={{ once: true, margin: "-60px" }}
          transition={{ duration: 0.2, ease }}
          className="flex flex-col items-center gap-6"
        >
          <h2 className="font-display text-display-md text-on-primary leading-tight">
            Start Automating Today
          </h2>
          <p className="font-body text-body-lg text-on-primary/80 max-w-[46ch]">
            Open-source, self-hostable, and production-ready. Deploy the backend
            with Docker in minutes and start pairing your first device.
          </p>
        </motion.div>

        <motion.div
          initial={{ opacity: 0, y: 12 }}
          whileInView={{ opacity: 1, y: 0 }}
          viewport={{ once: true }}
          transition={{ duration: 0.2, ease, delay: 0.08 }}
          className="flex flex-wrap gap-4 justify-center"
        >
          {/* Ghost CTA on gradient background */}
          <a
            href="https://github.com"
            rel="noopener noreferrer"
            className="inline-flex items-center gap-2 border border-on-primary/30 text-on-primary font-body font-semibold text-label-lg px-7 py-3 rounded-full transition-all duration-[200ms] hover:bg-on-primary/10"
          >
            <GitHubIcon />
            View on GitHub
          </a>
          <a
            href="#features"
            className="inline-flex items-center gap-2 bg-on-primary text-primary-container font-body font-semibold text-label-lg px-7 py-3 rounded-full transition-all duration-[200ms] hover:opacity-90"
          >
            Explore Features
          </a>
        </motion.div>
      </div>
    </section>
  );
}

function GitHubIcon() {
  return (
    <svg width="16" height="16" viewBox="0 0 16 16" fill="currentColor">
      <path d="M8 0C3.58 0 0 3.58 0 8c0 3.54 2.29 6.53 5.47 7.59.4.07.55-.17.55-.38 0-.19-.01-.82-.01-1.49-2.01.37-2.53-.49-2.69-.94-.09-.23-.48-.94-.82-1.13-.28-.15-.68-.52-.01-.53.63-.01 1.08.58 1.23.82.72 1.21 1.87.87 2.33.66.07-.52.28-.87.51-1.07-1.78-.2-3.64-.89-3.64-3.95 0-.87.31-1.59.82-2.15-.08-.2-.36-1.02.08-2.12 0 0 .67-.21 2.2.82.64-.18 1.32-.27 2-.27.68 0 1.36.09 2 .27 1.53-1.04 2.2-.82 2.2-.82.44 1.1.16 1.92.08 2.12.51.56.82 1.27.82 2.15 0 3.07-1.87 3.75-3.65 3.95.29.25.54.73.54 1.48 0 1.07-.01 1.93-.01 2.2 0 .21.15.46.55.38A8.013 8.013 0 0016 8c0-4.42-3.58-8-8-8z" />
    </svg>
  );
}

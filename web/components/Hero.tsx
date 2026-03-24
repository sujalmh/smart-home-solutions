"use client";

import { motion } from "framer-motion";

const ease = [0.4, 0, 0.2, 1] as const;

export default function Hero() {
  return (
    <section className="relative min-h-screen flex items-center overflow-hidden bg-background">
      {/* Ambient background glow */}
      <div
        aria-hidden
        className="pointer-events-none absolute top-[-20%] left-[-10%] w-[60vw] h-[60vh] rounded-full opacity-10"
        style={{
          background: "radial-gradient(circle, #00daf3 0%, transparent 70%)",
          filter: "blur(80px)",
        }}
      />
      <div
        aria-hidden
        className="pointer-events-none absolute bottom-[-10%] right-[-5%] w-[40vw] h-[40vh] rounded-full opacity-8"
        style={{
          background: "radial-gradient(circle, #009fb2 0%, transparent 70%)",
          filter: "blur(100px)",
        }}
      />

      <div className="relative z-10 max-w-7xl mx-auto px-6 lg:px-10 w-full pt-24 pb-16">
        <div className="grid grid-cols-1 lg:grid-cols-2 gap-12 lg:gap-20 items-center">
          {/* LEFT: Copy */}
          <motion.div
            initial={{ opacity: 0, y: 32 }}
            animate={{ opacity: 1, y: 0 }}
            transition={{ duration: 0.5, ease }}
            className="flex flex-col gap-8"
          >
            {/* Eyebrow chip */}
            <div className="inline-flex items-center gap-2 self-start bg-surface-container-highest px-3 py-1.5 rounded-full">
              <span className="w-1.5 h-1.5 rounded-full bg-primary animate-pulse" />
              <span className="text-label-sm font-body text-primary uppercase tracking-[0.05rem]">
                Real-time Automation
              </span>
            </div>

            {/* Headline */}
            <h1 className="font-display text-display-lg text-on-surface leading-[1.05]">
              Control Every Room.{" "}
              <span
                className="text-transparent bg-clip-text"
                style={{
                  backgroundImage: "linear-gradient(135deg, #00daf3 0%, #7bd1fa 100%)",
                }}
              >
                Intelligently.
              </span>
            </h1>

            {/* Sub-headline */}
            <p className="font-body text-body-lg text-on-surface-variant max-w-[44ch] leading-relaxed">
              A unified smart home platform connecting every switch, sensor, and
              scene — orchestrated by real-time WebSockets and an AI assistant
              that understands natural language.
            </p>

            {/* CTAs */}
            <div className="flex flex-wrap gap-4">
              <a
                href="#features"
                className="gradient-cta font-body font-semibold text-label-lg px-7 py-3 rounded-full inline-flex items-center gap-2"
              >
                Get Started
                <ArrowRight />
              </a>
              <a
                href="https://github.com"
                className="btn-ghost font-body font-medium text-label-lg px-7 py-3 rounded-full inline-flex items-center gap-2"
                rel="noopener noreferrer"
              >
                View on GitHub
              </a>
            </div>

            {/* Stats row */}
            <div className="flex gap-8 pt-2">
              {[
                { value: "4+", label: "Device Types" },
                { value: "Real-time", label: "WebSocket Sync" },
                { value: "AI", label: "LangGraph NLU" },
              ].map((stat) => (
                <div key={stat.label} className="flex flex-col gap-1">
                  <span className="font-display font-bold text-title-lg text-primary">
                    {stat.value}
                  </span>
                  <span className="font-body text-label-sm text-on-surface-variant uppercase tracking-[0.05rem]">
                    {stat.label}
                  </span>
                </div>
              ))}
            </div>
          </motion.div>

          {/* RIGHT: Hero visual */}
          <motion.div
            initial={{ opacity: 0, y: 24 }}
            animate={{ opacity: 1, y: 0 }}
            transition={{ duration: 0.6, ease, delay: 0.15 }}
            className="relative flex flex-col items-center justify-center"
          >
            {/* Large hero metric */}
            <div className="relative z-10 w-full bg-surface-container-low rounded-xl p-8 flex flex-col gap-6">
              {/* Metric card */}
              <div className="flex items-start justify-between">
                <div className="flex flex-col gap-1">
                  <span className="font-body text-label-sm text-on-surface-variant uppercase tracking-[0.05rem]">
                    Current Home Temperature
                  </span>
                  <span
                    className="font-display leading-none"
                    style={{
                      fontSize: "3.5rem",
                      fontWeight: 700,
                      color: "#00daf3",
                    }}
                  >
                    22°C
                  </span>
                </div>
                <div className="flex flex-col gap-1.5 items-end">
                  <div className="flex items-center gap-1.5 bg-surface-container-highest px-2.5 py-1 rounded-full">
                    <span className="w-1.5 h-1.5 rounded-full bg-primary" />
                    <span className="font-body text-label-sm text-primary uppercase tracking-[0.05rem]">Active</span>
                  </div>
                  <span className="font-body text-label-sm text-on-surface-variant">6 devices online</span>
                </div>
              </div>

              {/* Blueprint floor plan SVG */}
              <div className="relative rounded-md overflow-hidden bg-background/60 p-4">
                <FloorPlanSVG />
              </div>

              {/* Room status row */}
              <div className="grid grid-cols-3 gap-3">
                {[
                  { room: "Living Room", devices: 4, on: true },
                  { room: "Bedroom", devices: 2, on: true },
                  { room: "Kitchen", devices: 3, on: false },
                ].map((r) => (
                  <div
                    key={r.room}
                    className="bg-surface-container-highest rounded p-3 flex flex-col gap-1"
                  >
                    <div className="flex items-center gap-1.5">
                      <span
                        className={`w-1.5 h-1.5 rounded-full ${
                          r.on ? "bg-primary" : "bg-outline-variant"
                        }`}
                      />
                      <span className="font-body text-label-sm text-on-surface-variant uppercase tracking-[0.05rem]">
                        {r.on ? "On" : "Off"}
                      </span>
                    </div>
                    <span className="font-body text-body-sm text-on-surface">{r.room}</span>
                    <span className="font-body text-label-sm text-on-surface-variant">
                      {r.devices} device{r.devices !== 1 ? "s" : ""}
                    </span>
                  </div>
                ))}
              </div>
            </div>
          </motion.div>
        </div>
      </div>
    </section>
  );
}

function ArrowRight() {
  return (
    <svg width="14" height="14" viewBox="0 0 14 14" fill="none">
      <path d="M1 7h12M7 1l6 6-6 6" stroke="currentColor" strokeWidth="1.5" strokeLinecap="round" strokeLinejoin="round" />
    </svg>
  );
}

function FloorPlanSVG() {
  return (
    <svg
      viewBox="0 0 320 180"
      fill="none"
      xmlns="http://www.w3.org/2000/svg"
      className="w-full blueprint-glow"
    >
      {/* Outer walls */}
      <rect x="10" y="10" width="300" height="160" rx="2" stroke="#00daf3" strokeWidth="1.5" />

      {/* Interior walls */}
      <line x1="160" y1="10" x2="160" y2="110" stroke="#00daf3" strokeWidth="1" />
      <line x1="10" y1="110" x2="260" y2="110" stroke="#00daf3" strokeWidth="1" />
      <line x1="260" y1="110" x2="260" y2="10" stroke="#00daf3" strokeWidth="1" />

      {/* Doors (arc notation) */}
      <path d="M160 90 Q175 90 175 75" stroke="#00daf3" strokeWidth="0.75" strokeDasharray="2 2" />
      <path d="M240 110 Q240 125 255 125" stroke="#00daf3" strokeWidth="0.75" strokeDasharray="2 2" />

      {/* Window markers */}
      <line x1="80" y1="10" x2="120" y2="10" stroke="#7bd1fa" strokeWidth="2" />
      <line x1="200" y1="10" x2="240" y2="10" stroke="#7bd1fa" strokeWidth="2" />
      <line x1="310" y1="50" x2="310" y2="90" stroke="#7bd1fa" strokeWidth="2" />

      {/* Room labels */}
      <text x="72" y="65" fill="#00daf3" fontSize="9" fontFamily="monospace" opacity="0.9">LIVING ROOM</text>
      <text x="195" y="65" fill="#00daf3" fontSize="9" fontFamily="monospace" opacity="0.9">BEDROOM</text>
      <text x="30" y="145" fill="#00daf3" fontSize="9" fontFamily="monospace" opacity="0.9">KITCHEN</text>
      <text x="195" y="145" fill="#00daf3" fontSize="9" fontFamily="monospace" opacity="0.9">BATHROOM</text>

      {/* Device dots */}
      <circle cx="90" cy="75" r="3" fill="#00daf3" opacity="0.7" />
      <circle cx="110" cy="55" r="3" fill="#00daf3" opacity="0.7" />
      <circle cx="220" cy="60" r="3" fill="#00daf3" opacity="0.7" />
      <circle cx="210" cy="45" r="3" fill="#00daf3" opacity="0.7" />
      <circle cx="60" cy="130" r="3" fill="#009fb2" opacity="0.5" />
      <circle cx="220" cy="135" r="3" fill="#009fb2" opacity="0.5" />

      {/* Cross-hair coordinate annotations */}
      <line x1="10" y1="170" x2="310" y2="170" stroke="#414754" strokeWidth="0.5" />
      <text x="155" y="178" fill="#414754" fontSize="7" fontFamily="monospace" textAnchor="middle">12.0m</text>
      <line x1="2" y1="10" x2="2" y2="170" stroke="#414754" strokeWidth="0.5" />
      <text x="0" y="95" fill="#414754" fontSize="7" fontFamily="monospace" textAnchor="middle" transform="rotate(-90,0,95)">8.0m</text>
    </svg>
  );
}

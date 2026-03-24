"use client";

import { motion } from "framer-motion";

const ease = [0.4, 0, 0.2, 1] as const;

const features = [
  {
    icon: <DeviceIcon />,
    chip: "Active",
    title: "Device Control",
    description:
      "Manage smart switches with granular on/off, manual/auto modes, and load-level slider (0–1000) — all synced in real time across every client.",
  },
  {
    icon: <RoomIcon />,
    chip: "Organized",
    title: "Room Management",
    description:
      "Structure your home as Homes → Rooms → Devices. Bulk-control entire rooms or drill down to individual channels with a tap.",
  },
  {
    icon: <SyncIcon />,
    chip: "Live",
    title: "Real-time Sync",
    description:
      "Bidirectional WebSocket events dispatch commands and surface device responses in under 50ms — no polling, no stale state.",
  },
  {
    icon: <AIIcon />,
    chip: "Beta",
    title: "AI Assistant",
    description:
      "LangGraph-powered NLU understands intent and entities in natural language — turn on lights, set scenes, or query energy usage conversationally.",
  },
];

export default function Features() {
  return (
    <section id="features" className="bg-surface-container-low py-section-lg">
      <div className="max-w-7xl mx-auto px-6 lg:px-10">
        {/* Section header */}
        <div className="flex flex-col gap-4 mb-16">
          <span className="font-body text-label-sm text-primary uppercase tracking-[0.05rem]">
            Platform Capabilities
          </span>
          <h2 className="font-display text-display-md text-on-surface max-w-[24ch]">
            Everything Your Home Needs
          </h2>
          <p className="font-body text-body-lg text-on-surface-variant max-w-[52ch]">
            Built on a production-grade stack — not a prototype. Every feature
            is designed for reliability, extensibility, and day-to-day use.
          </p>
        </div>

        {/* Feature grid */}
        <div className="grid grid-cols-1 sm:grid-cols-2 lg:grid-cols-4 gap-6">
          {features.map((feat, i) => (
            <motion.div
              key={feat.title}
              initial={{ opacity: 0, y: 24 }}
              whileInView={{ opacity: 1, y: 0 }}
              viewport={{ once: true, margin: "-60px" }}
              transition={{ duration: 0.2, ease, delay: i * 0.07 }}
              className="group bg-surface-container-high hover:bg-surface-container-highest rounded-lg p-6 flex flex-col gap-5 transition-colors duration-[200ms] cursor-default"
            >
              {/* Icon */}
              <div className="w-10 h-10 rounded-sm bg-primary/10 flex items-center justify-center text-primary">
                {feat.icon}
              </div>

              {/* Status chip */}
              <div className="flex items-center gap-1.5 self-start bg-surface-container px-2.5 py-1 rounded-full">
                <span className="w-1 h-1 rounded-full bg-primary" />
                <span className="font-body text-label-sm text-on-surface-variant uppercase tracking-[0.05rem]">
                  {feat.chip}
                </span>
              </div>

              {/* Title */}
              <h3 className="font-display text-title-md text-on-surface">
                {feat.title}
              </h3>

              {/* Description */}
              <p className="font-body text-body-md text-on-surface-variant leading-relaxed flex-1">
                {feat.description}
              </p>
            </motion.div>
          ))}
        </div>
      </div>
    </section>
  );
}

function DeviceIcon() {
  return (
    <svg width="20" height="20" viewBox="0 0 20 20" fill="none">
      <rect x="3" y="3" width="14" height="14" rx="2" stroke="currentColor" strokeWidth="1.5" />
      <circle cx="10" cy="10" r="2.5" stroke="currentColor" strokeWidth="1.5" />
      <line x1="10" y1="5" x2="10" y2="7.5" stroke="currentColor" strokeWidth="1.5" strokeLinecap="round" />
    </svg>
  );
}

function RoomIcon() {
  return (
    <svg width="20" height="20" viewBox="0 0 20 20" fill="none">
      <path d="M2 17V8l8-5 8 5v9" stroke="currentColor" strokeWidth="1.5" strokeLinejoin="round" />
      <rect x="7" y="12" width="6" height="5" rx="0.5" stroke="currentColor" strokeWidth="1.5" />
    </svg>
  );
}

function SyncIcon() {
  return (
    <svg width="20" height="20" viewBox="0 0 20 20" fill="none">
      <path d="M4 10a6 6 0 0 1 10.5-4" stroke="currentColor" strokeWidth="1.5" strokeLinecap="round" />
      <path d="M16 10a6 6 0 0 1-10.5 4" stroke="currentColor" strokeWidth="1.5" strokeLinecap="round" />
      <polyline points="14,4 16.5,6 14,8" stroke="currentColor" strokeWidth="1.5" strokeLinecap="round" strokeLinejoin="round" />
      <polyline points="6,12 3.5,14 6,16" stroke="currentColor" strokeWidth="1.5" strokeLinecap="round" strokeLinejoin="round" />
    </svg>
  );
}

function AIIcon() {
  return (
    <svg width="20" height="20" viewBox="0 0 20 20" fill="none">
      <circle cx="10" cy="10" r="7" stroke="currentColor" strokeWidth="1.5" />
      <path d="M7 10h6M10 7v6" stroke="currentColor" strokeWidth="1.5" strokeLinecap="round" />
      <circle cx="10" cy="10" r="2" fill="currentColor" opacity="0.3" />
    </svg>
  );
}

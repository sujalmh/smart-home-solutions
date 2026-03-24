"use client";

import { motion } from "framer-motion";

const ease = [0.4, 0, 0.2, 1] as const;

const chips = [
  { label: "FastAPI", category: "Backend" },
  { label: "PostgreSQL", category: "Database" },
  { label: "SQLAlchemy 2.0", category: "ORM" },
  { label: "Alembic", category: "Migrations" },
  { label: "WebSocket", category: "Transport" },
  { label: "Flutter", category: "Mobile" },
  { label: "Riverpod", category: "State" },
  { label: "ESP32", category: "Hardware" },
  { label: "WemosD1", category: "Hardware" },
  { label: "LangGraph", category: "AI" },
  { label: "gpt-4o-mini", category: "AI" },
  { label: "JWT + bcrypt", category: "Auth" },
  { label: "Docker", category: "Deploy" },
  { label: "AWS Lightsail", category: "Cloud" },
];

const categoryColors: Record<string, string> = {
  Backend: "#00daf3",
  Database: "#7bd1fa",
  ORM: "#7bd1fa",
  Migrations: "#7bd1fa",
  Transport: "#00daf3",
  Mobile: "#00daf3",
  State: "#009fb2",
  Hardware: "#7bd1fa",
  AI: "#00daf3",
  Auth: "#009fb2",
  Deploy: "#7bd1fa",
  Cloud: "#7bd1fa",
};

export default function TechStack() {
  return (
    <section id="tech" className="bg-background py-section-lg">
      <div className="max-w-7xl mx-auto px-6 lg:px-10">
        <div className="flex flex-col items-center gap-14">
          {/* Header */}
          <motion.div
            initial={{ opacity: 0, y: 20 }}
            whileInView={{ opacity: 1, y: 0 }}
            viewport={{ once: true, margin: "-60px" }}
            transition={{ duration: 0.2, ease }}
            className="flex flex-col items-center gap-4 text-center"
          >
            <span className="font-body text-label-sm text-primary uppercase tracking-[0.05rem]">
              Under the Hood
            </span>
            <h2 className="font-display text-display-md text-on-surface">
              Built on a Modern Stack
            </h2>
            <p className="font-body text-body-lg text-on-surface-variant max-w-[46ch]">
              Each layer of the platform was chosen for production reliability — from
              IoT firmware to AI orchestration.
            </p>
          </motion.div>

          {/* Chip grid */}
          <motion.div
            initial={{ opacity: 0 }}
            whileInView={{ opacity: 1 }}
            viewport={{ once: true, margin: "-60px" }}
            transition={{ duration: 0.3, ease, delay: 0.08 }}
            className="flex flex-wrap justify-center gap-3"
          >
            {chips.map((chip, i) => (
              <motion.div
                key={chip.label}
                initial={{ opacity: 0, scale: 0.95 }}
                whileInView={{ opacity: 1, scale: 1 }}
                viewport={{ once: true }}
                transition={{ duration: 0.2, ease, delay: i * 0.03 }}
                className="flex items-center gap-2 bg-surface-container-highest px-4 py-2.5 rounded-full cursor-default group hover:bg-surface-container-high transition-colors duration-[200ms]"
              >
                <span
                  className="w-1.5 h-1.5 rounded-full flex-shrink-0"
                  style={{ background: categoryColors[chip.category] ?? "#00daf3" }}
                />
                <span className="font-body text-label-sm text-on-surface uppercase tracking-[0.05rem]">
                  {chip.label}
                </span>
                <span className="font-body text-label-sm text-outline uppercase tracking-[0.04rem] opacity-60">
                  {chip.category}
                </span>
              </motion.div>
            ))}
          </motion.div>

          {/* Architecture note */}
          <motion.div
            initial={{ opacity: 0, y: 12 }}
            whileInView={{ opacity: 1, y: 0 }}
            viewport={{ once: true }}
            transition={{ duration: 0.2, ease, delay: 0.12 }}
            className="max-w-2xl w-full bg-surface-container-low rounded-lg px-6 py-5 flex items-start gap-4"
          >
            <span className="text-primary mt-0.5 flex-shrink-0">
              <svg width="16" height="16" viewBox="0 0 16 16" fill="none">
                <circle cx="8" cy="8" r="7" stroke="currentColor" strokeWidth="1.5" />
                <path d="M8 5v3.5L10.5 11" stroke="currentColor" strokeWidth="1.5" strokeLinecap="round" />
              </svg>
            </span>
            <p className="font-body text-body-md text-on-surface-variant leading-relaxed">
              The system uses a full real-time WebSocket bus — not REST polling —
              so device state changes appear on every connected client (mobile, web, legacy Android)
              within milliseconds, regardless of which client sent the command.
            </p>
          </motion.div>
        </div>
      </div>
    </section>
  );
}

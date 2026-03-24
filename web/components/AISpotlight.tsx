"use client";

import { motion } from "framer-motion";

const ease = [0.4, 0, 0.2, 1] as const;

const conversation = [
  { role: "user", text: "Hey, can you set the living room to movie mode?" },
  { role: "ai", text: "Sure — dimming living room lights to 30% and closing the smart blinds.", label: "control_device · 3 actions" },
  { role: "user", text: "What's today's energy usage so far?" },
  { role: "ai", text: "You've consumed 4.2 kWh today. Kitchen is your top draw at 1.8 kWh.", label: "analytics_query" },
];

export default function AISpotlight() {
  return (
    <section id="ai" className="bg-surface-container-low py-section-lg">
      <div className="max-w-7xl mx-auto px-6 lg:px-10">
        <div className="grid grid-cols-1 lg:grid-cols-11 gap-12 lg:gap-20 items-center">
          {/* LEFT: 6/11 — copy */}
          <motion.div
            initial={{ opacity: 0, y: 24 }}
            whileInView={{ opacity: 1, y: 0 }}
            viewport={{ once: true, margin: "-60px" }}
            transition={{ duration: 0.2, ease }}
            className="lg:col-span-6 flex flex-col gap-7"
          >
            <span className="font-body text-label-sm text-primary uppercase tracking-[0.05rem]">
              AI Assistant · Beta
            </span>

            <h2 className="font-display text-display-md text-on-surface leading-tight">
              Your Home{" "}
              <span
                className="text-transparent bg-clip-text"
                style={{ backgroundImage: "linear-gradient(135deg, #00daf3 0%, #7bd1fa 100%)" }}
              >
                Understands You
              </span>
            </h2>

            <p className="font-body text-body-lg text-on-surface-variant leading-relaxed max-w-[46ch]">
              Built on LangGraph with OpenAI&apos;s gpt-4o-mini at its core, the AI assistant
              interprets natural language across multi-turn conversations to control devices,
              query analytics, and manage automations — with safety guardrails at every step.
            </p>

            <div className="grid grid-cols-1 sm:grid-cols-2 gap-4">
              {[
                { label: "Natural Language Control", desc: "Phrase commands in plain English — intents and entities are extracted automatically." },
                { label: "Multi-turn Context", desc: "The assistant remembers conversation history to handle follow-up questions naturally." },
                { label: "Safety Guardrails", desc: "All commands pass through invariant checks before execution to prevent unintended states." },
                { label: "Analytics Queries", desc: "Ask for energy summaries, device uptime leaderboards, or trend analysis by room." },
              ].map((cap) => (
                <div key={cap.label} className="flex flex-col gap-1.5">
                  <div className="flex items-center gap-2">
                    <span className="w-1.5 h-1.5 rounded-full bg-primary flex-shrink-0" />
                    <span className="font-body text-label-md text-on-surface font-medium">{cap.label}</span>
                  </div>
                  <p className="font-body text-body-sm text-on-surface-variant pl-3.5 leading-relaxed">
                    {cap.desc}
                  </p>
                </div>
              ))}
            </div>
          </motion.div>

          {/* RIGHT: 5/11 — glass chat card */}
          <motion.div
            initial={{ opacity: 0, y: 24 }}
            whileInView={{ opacity: 1, y: 0 }}
            viewport={{ once: true, margin: "-60px" }}
            transition={{ duration: 0.2, ease, delay: 0.1 }}
            className="lg:col-span-5"
          >
            <div className="glass rounded-xl overflow-hidden shadow-float-lg">
              {/* Chat header */}
              <div className="flex items-center gap-3 px-5 py-4 bg-surface-container-highest/60">
                <div className="w-7 h-7 rounded-full bg-primary/20 flex items-center justify-center">
                  <span className="text-primary text-xs">AI</span>
                </div>
                <div className="flex flex-col">
                  <span className="font-body text-label-md text-on-surface font-medium">Home Assistant</span>
                  <span className="font-body text-label-sm text-primary uppercase tracking-[0.04rem]">Online</span>
                </div>
                <div className="ml-auto flex items-center gap-1.5">
                  <span className="w-2 h-2 rounded-full bg-primary animate-pulse-slow" />
                </div>
              </div>

              {/* Messages */}
              <div className="px-5 py-5 flex flex-col gap-4">
                {conversation.map((msg, i) => (
                  <motion.div
                    key={i}
                    initial={{ opacity: 0, y: 8 }}
                    whileInView={{ opacity: 1, y: 0 }}
                    viewport={{ once: true }}
                    transition={{ duration: 0.2, ease, delay: i * 0.06 }}
                    className={`flex flex-col gap-1 ${msg.role === "user" ? "items-end" : "items-start"}`}
                  >
                    <div
                      className={`max-w-[85%] px-3.5 py-2.5 rounded-md ${
                        msg.role === "user"
                          ? "bg-surface-container-highest text-on-surface"
                          : "bg-primary/10 border border-primary/20 text-on-surface"
                      }`}
                    >
                      <p className="font-body text-body-sm leading-relaxed">{msg.text}</p>
                    </div>
                    {msg.role === "ai" && msg.label && (
                      <span className="font-body text-label-sm text-on-surface-variant uppercase tracking-[0.04rem] pl-1">
                        {msg.label}
                      </span>
                    )}
                  </motion.div>
                ))}
              </div>

              {/* Input bar */}
              <div className="px-5 pb-5">
                <div className="flex items-center gap-3 bg-surface-container-highest rounded px-4 py-3">
                  <span className="font-body text-body-sm text-outline-variant flex-1">
                    Ask your home anything...
                  </span>
                  <button className="w-7 h-7 rounded-sm gradient-cta flex items-center justify-center">
                    <svg width="12" height="12" viewBox="0 0 12 12" fill="none">
                      <path d="M1 11L11 1M11 1H4M11 1v7" stroke="currentColor" strokeWidth="1.5" strokeLinecap="round" strokeLinejoin="round" />
                    </svg>
                  </button>
                </div>
              </div>
            </div>
          </motion.div>
        </div>
      </div>
    </section>
  );
}

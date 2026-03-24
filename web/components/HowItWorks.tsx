"use client";

import { motion } from "framer-motion";

const ease = [0.4, 0, 0.2, 1] as const;

const steps = [
  {
    number: "01",
    title: "Connect Your Devices",
    detail:
      "Flash the WemosD1 slave firmware and pair each switch module to the ESP32 master gateway. The gateway joins your local network and establishes a persistent WebSocket tunnel to the backend.",
    annotation: "ESP32 + WemosD1 Firmware · WebSocket",
  },
  {
    number: "02",
    title: "Organize Rooms & Scenes",
    detail:
      "Log in with your account, create a Home, then group devices into Rooms. Assign names, icons, and default modes (manual / auto). Changes propagate to every connected client instantly.",
    annotation: "JWT Auth · Flutter App · Real-time Sync",
  },
  {
    number: "03",
    title: "Talk to Your Home",
    detail:
      "Ask the AI assistant anything: 'Turn off all bedroom lights' or 'What is the kitchen power usage today?' LangGraph resolves intent, extracts entities, and dispatches the right device commands — safely.",
    annotation: "LangGraph · gpt-4o-mini · Safety Controls",
  },
];

export default function HowItWorks() {
  return (
    <section id="how-it-works" className="bg-background py-section-lg overflow-hidden">
      <div className="max-w-7xl mx-auto px-6 lg:px-10">
        {/* === Section header === */}
        <div className="flex flex-col gap-4 mb-20">
          <span className="font-body text-label-sm text-primary uppercase tracking-[0.05rem]">
            Setup in Minutes
          </span>
          <h2 className="font-display text-display-md text-on-surface">
            How It Works
          </h2>
        </div>

        {/* === Steps: asymmetric 60/40 layout === */}
        <div className="flex flex-col gap-16">
          {steps.map((step, i) => (
            <motion.div
              key={step.number}
              initial={{ opacity: 0, y: 24 }}
              whileInView={{ opacity: 1, y: 0 }}
              viewport={{ once: true, margin: "-60px" }}
              transition={{ duration: 0.2, ease, delay: i * 0.07 }}
              className={`grid grid-cols-1 lg:grid-cols-5 gap-8 lg:gap-16 items-start ${
                i % 2 === 1 ? "lg:grid-flow-dense" : ""
              }`}
            >
              {/* Left: step number + diagram — 3/5 width */}
              <div className={`lg:col-span-3 flex gap-6 items-start ${i % 2 === 1 ? "lg:col-start-3" : ""}`}>
                {/* Step number circle */}
                <div className="flex-shrink-0 flex flex-col items-center gap-3">
                  <div className="w-12 h-12 rounded-full bg-primary/10 border border-primary/20 flex items-center justify-center">
                    <span className="font-display text-display-sm text-primary leading-none">
                      {step.number}
                    </span>
                  </div>
                  {i < steps.length - 1 && (
                    <div className="w-px flex-1 min-h-[80px] bg-gradient-to-b from-primary/30 to-transparent" />
                  )}
                </div>

                {/* Diagram placeholder */}
                <div className="flex-1 bg-surface-container-low rounded-lg p-6 min-h-[160px] flex items-center justify-center">
                  <StepDiagram index={i} />
                </div>
              </div>

              {/* Right: text block — 2/5 width */}
              <div className={`lg:col-span-2 flex flex-col gap-4 pt-2 ${i % 2 === 1 ? "lg:col-start-1 lg:row-start-1" : ""}`}>
                <h3 className="font-display text-title-lg text-on-surface">
                  {step.title}
                </h3>
                <p className="font-body text-body-md text-on-surface-variant leading-relaxed">
                  {step.detail}
                </p>
                <span className="font-body text-label-sm text-outline-variant uppercase tracking-[0.05rem]">
                  {step.annotation}
                </span>
              </div>
            </motion.div>
          ))}
        </div>
      </div>
    </section>
  );
}

function StepDiagram({ index }: { index: number }) {
  if (index === 0) {
    return (
      <svg viewBox="0 0 280 120" fill="none" className="w-full max-w-xs blueprint-glow">
        {/* ESP32 master box */}
        <rect x="20" y="35" width="70" height="50" rx="3" stroke="#00daf3" strokeWidth="1.5" />
        <text x="55" y="57" fill="#00daf3" fontSize="7" fontFamily="monospace" textAnchor="middle">ESP32</text>
        <text x="55" y="68" fill="#00daf3" fontSize="7" fontFamily="monospace" textAnchor="middle">MASTER</text>

        {/* Connection lines */}
        <line x1="90" y1="60" x2="120" y2="60" stroke="#00daf3" strokeWidth="1" strokeDasharray="3 2" />
        <line x1="120" y1="60" x2="120" y2="30" stroke="#00daf3" strokeWidth="1" />
        <line x1="120" y1="60" x2="120" y2="90" stroke="#00daf3" strokeWidth="1" />
        <line x1="120" y1="30" x2="150" y2="30" stroke="#00daf3" strokeWidth="1" />
        <line x1="120" y1="60" x2="150" y2="60" stroke="#00daf3" strokeWidth="1" />
        <line x1="120" y1="90" x2="150" y2="90" stroke="#00daf3" strokeWidth="1" />

        {/* Slave boxes */}
        {[30, 60, 90].map((y) => (
          <g key={y}>
            <rect x="150" y={y - 12} width="50" height="24" rx="2" stroke="#009fb2" strokeWidth="1" />
            <text x="175" y={y + 4} fill="#009fb2" fontSize="7" fontFamily="monospace" textAnchor="middle">D1 CH{(y - 30) / 30 + 1}</text>
          </g>
        ))}

        {/* Backend cloud */}
        <rect x="220" y="45" width="45" height="30" rx="4" stroke="#7bd1fa" strokeWidth="1" strokeDasharray="2 2" />
        <text x="242" y="58" fill="#7bd1fa" fontSize="6" fontFamily="monospace" textAnchor="middle">BACKEND</text>
        <text x="242" y="68" fill="#7bd1fa" fontSize="6" fontFamily="monospace" textAnchor="middle">API</text>
        <line x1="200" y1="60" x2="220" y2="60" stroke="#7bd1fa" strokeWidth="0.75" />

        {/* Labels */}
        <text x="55" y="98" fill="#414754" fontSize="6" fontFamily="monospace" textAnchor="middle">Gateway</text>
        <text x="175" y="109" fill="#414754" fontSize="6" fontFamily="monospace" textAnchor="middle">Slave Nodes</text>
      </svg>
    );
  }

  if (index === 1) {
    return (
      <svg viewBox="0 0 280 120" fill="none" className="w-full max-w-xs blueprint-glow">
        {/* Home hierarchy */}
        <rect x="110" y="8" width="60" height="22" rx="2" stroke="#00daf3" strokeWidth="1.5" />
        <text x="140" y="22" fill="#00daf3" fontSize="8" fontFamily="monospace" textAnchor="middle">HOME</text>

        <line x1="140" y1="30" x2="90" y2="50" stroke="#00daf3" strokeWidth="1" />
        <line x1="140" y1="30" x2="190" y2="50" stroke="#00daf3" strokeWidth="1" />

        <rect x="60" y="50" width="60" height="22" rx="2" stroke="#009fb2" strokeWidth="1" />
        <text x="90" y="64" fill="#009fb2" fontSize="7" fontFamily="monospace" textAnchor="middle">LIVING RM</text>
        <rect x="160" y="50" width="60" height="22" rx="2" stroke="#009fb2" strokeWidth="1" />
        <text x="190" y="64" fill="#009fb2" fontSize="7" fontFamily="monospace" textAnchor="middle">BEDROOM</text>

        <line x1="90" y1="72" x2="70" y2="92" stroke="#009fb2" strokeWidth="0.75" />
        <line x1="90" y1="72" x2="110" y2="92" stroke="#009fb2" strokeWidth="0.75" />
        <line x1="190" y1="72" x2="190" y2="92" stroke="#009fb2" strokeWidth="0.75" />

        {[
          [55, 92, "SW 01"],
          [95, 92, "SW 02"],
          [175, 92, "SW 03"],
        ].map(([x, y, label]) => (
          <g key={String(label)}>
            <rect x={Number(x)} y={Number(y)} width="30" height="16" rx="1.5" stroke="#414754" strokeWidth="0.75" />
            <text x={Number(x) + 15} y={Number(y) + 11} fill="#c2cbe7" fontSize="6" fontFamily="monospace" textAnchor="middle">{label}</text>
          </g>
        ))}
      </svg>
    );
  }

  return (
    <svg viewBox="0 0 280 120" fill="none" className="w-full max-w-xs blueprint-glow">
      {/* Chat UI mock */}
      <rect x="20" y="10" width="240" height="100" rx="4" stroke="#1a2540" strokeWidth="0" fill="#111c35" />

      {/* User bubble */}
      <rect x="120" y="20" width="130" height="24" rx="3" fill="#2a3a5c" />
      <text x="185" y="35" fill="#dae2fd" fontSize="7.5" fontFamily="monospace" textAnchor="middle">Turn off bedroom lights</text>

      {/* AI bubble */}
      <rect x="30" y="54" width="160" height="24" rx="3" fill="#1a2540" />
      <rect x="30" y="54" width="160" height="24" rx="3" stroke="#00daf3" strokeWidth="0.5" />
      <text x="110" y="65" fill="#00daf3" fontSize="7" fontFamily="monospace" textAnchor="middle">Intent: control_device</text>
      <text x="110" y="73" fill="#7bd1fa" fontSize="6.5" fontFamily="monospace" textAnchor="middle">✓ Bedroom: 2 devices off</text>

      {/* Typing indicator */}
      <circle cx="38" cy="98" r="2" fill="#00daf3" opacity="0.4" />
      <circle cx="46" cy="98" r="2" fill="#00daf3" opacity="0.65" />
      <circle cx="54" cy="98" r="2" fill="#00daf3" opacity="1" />
    </svg>
  );
}

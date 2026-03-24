export default function Footer() {
  const year = new Date().getFullYear();

  return (
    <footer className="bg-surface-container-low py-10">
      <div className="max-w-7xl mx-auto px-6 lg:px-10 flex flex-col sm:flex-row items-center justify-between gap-4">
        {/* Logo */}
        <div className="flex items-center gap-2.5">
          <span className="flex items-center justify-center w-7 h-7 rounded-sm bg-primary/10">
            <HomeSVG />
          </span>
          <span className="font-display font-semibold text-title-sm text-on-surface tracking-tight">
            Smart Home <span className="text-primary">Solutions</span>
          </span>
        </div>

        {/* Links */}
        <nav className="flex items-center gap-6">
          {[
            { label: "Features", href: "#features" },
            { label: "How It Works", href: "#how-it-works" },
            { label: "AI", href: "#ai" },
            { label: "Tech Stack", href: "#tech" },
          ].map((link) => (
            <a
              key={link.href}
              href={link.href}
              className="font-body text-label-sm text-on-surface-variant hover:text-on-surface transition-colors duration-[200ms] uppercase tracking-[0.05rem]"
            >
              {link.label}
            </a>
          ))}
        </nav>

        {/* Copyright */}
        <p className="font-body text-label-sm text-on-surface-variant uppercase tracking-[0.05rem]">
          © {year} Smart Home Solutions
        </p>
      </div>
    </footer>
  );
}

function HomeSVG() {
  return (
    <svg width="15" height="15" viewBox="0 0 18 18" fill="none">
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

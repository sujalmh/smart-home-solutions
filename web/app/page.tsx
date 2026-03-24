import Nav from "@/components/Nav";
import Hero from "@/components/Hero";
import Features from "@/components/Features";
import HowItWorks from "@/components/HowItWorks";
import AISpotlight from "@/components/AISpotlight";
import TechStack from "@/components/TechStack";
import CTABanner from "@/components/CTABanner";
import Footer from "@/components/Footer";

export default function Home() {
  return (
    <>
      <Nav />
      <main>
        <Hero />
        <Features />
        <HowItWorks />
        <AISpotlight />
        <TechStack />
        <CTABanner />
      </main>
      <Footer />
    </>
  );
}

import { CatFeederController } from "@/components/catFeederController";

export default function Home() {
  return (
    <div className="flex flex-col items-center justify-center min-h-screen p-8 md:p-20 font-[family-name:var(--font-geist-sans)]">
      <main className="w-full">
        <h1 className="text-3xl text-center mb-4">Food Dispensers</h1>
        <div className="flex flex-col items-center justify-center lg:flex-row gap-4">
          <CatFeederController
            initial={{ opacity: 0, y: -20 }}
            animate={{ opacity: 1, y: 0 }}
            transition={{ type: "spring" }}
            id="loki"
          />
          <CatFeederController
            initial={{ opacity: 0, y: -20 }}
            animate={{
              opacity: 1,
              y: 0,
            }}
            transition={{ delay: 0.1, type: "spring" }}
            id="gatito"
          />
        </div>
      </main>
    </div>
  );
}

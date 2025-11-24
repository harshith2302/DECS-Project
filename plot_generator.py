import pandas as pd
import matplotlib.pyplot as plt


files = {
    "results_get_only.csv": "get_only",
    "results_put_only.csv": "put_only",
    "results_mixed.csv": "mixed",
    "results_get_popular.csv": "get_popular"
}

for csv_file, label in files.items():
    print(f"Processing {csv_file} ...")
    

    df = pd.read_csv(csv_file)
    

    df["vus"] = pd.to_numeric(df["vus"], errors="coerce")
    df["tps"] = pd.to_numeric(df["tps"], errors="coerce")
    df["avg_ms"] = pd.to_numeric(df["avg_ms"], errors="coerce")
    
    plt.figure()
    plt.plot(df["vus"], df["tps"], marker="o")
    plt.xlabel("Virtual Users (VUs)")
    plt.ylabel("Throughput (req/s)")
    plt.title(f"Throughput vs VUs ({label})")
    plt.grid(True)
    plt.savefig(f"{label}_tps.png")
    plt.close()

    plt.figure()
    plt.plot(df["vus"], df["avg_ms"], marker="o")
    plt.xlabel("Virtual Users (VUs)")
    plt.ylabel("Average Response Time (ms)")
    plt.title(f"Avg Response Time vs VUs ({label})")
    plt.grid(True)
    plt.savefig(f"{label}_avg.png")
    plt.close()

print("All plots generated successfully.")

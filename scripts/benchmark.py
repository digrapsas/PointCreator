#!/usr/bin/env python3

import argparse
import csv
import re
import statistics
import subprocess
import time
from pathlib import Path

import matplotlib.pyplot as plt


# ---------------------------------------------------------------------------
# Configuration
# ---------------------------------------------------------------------------

EXECUTABLE = Path("build/pc.out")
INPUT_DIR = Path("input")
RESULTS_DIR = Path("benchmark")
RESULTS_FILE = RESULTS_DIR / "benchmark_results.csv"

THREAD_COUNTS = [1, 2, 3, 4, 6, 8, 16]
NUM_RUNS = 20
PAUSE_SECONDS = 0

NUM_POINTS = 10000000

CENTER_X = 0.0
CENTER_Y = 0.0
CENTER_Z = 0.0

RADIUS_MEAN = 1.0
RADIUS_STDDEV = 0.15


# ---------------------------------------------------------------------------
# Input generation
# ---------------------------------------------------------------------------

def create_input_file(num_threads: int) -> Path:
    INPUT_DIR.mkdir(parents=True, exist_ok=True)

    input_file = INPUT_DIR / f"input_{num_threads}.txt"

    content = f"""number of threads: {num_threads}
number of points:  {NUM_POINTS}

center x-coordinate: {CENTER_X}
center y-coordinate: {CENTER_Y}
center z-coordinate: {CENTER_Z}

radius mean value:  {RADIUS_MEAN}
standard deviation: {RADIUS_STDDEV}
"""

    input_file.write_text(content)

    return input_file


# ---------------------------------------------------------------------------
# Benchmark
# ---------------------------------------------------------------------------

def run_program(input_file: Path) -> float:
    result = subprocess.run(
        [str(EXECUTABLE), str(input_file)],
        capture_output=True,
        text=True,
        check=True,
    )

    match = re.search(
        r"Time taken to generate points:\s*([0-9]+(?:\.[0-9]+)?)\s*milliseconds",
        result.stdout,
    )

    if match is None:
        raise RuntimeError(
            "Could not find execution time in program output.\n"
            f"Program output:\n{result.stdout}"
        )

    return float(match.group(1))


def benchmark(num_threads: int, pause_seconds: float = PAUSE_SECONDS) -> list[float]:
    input_file = create_input_file(num_threads)

    print(f"\nBenchmarking {num_threads} thread(s)...")

    times = []

    for run in range(1, NUM_RUNS + 1):
        if pause_seconds > 0:
            print(f"  ...cooling for {pause_seconds} seconds...")
            time.sleep(pause_seconds)
        elapsed_ms = run_program(input_file)
        times.append(elapsed_ms)

        print(f"  Run {run}/{NUM_RUNS}: {elapsed_ms:.3f} ms")

    return times


# ---------------------------------------------------------------------------
# Results
# ---------------------------------------------------------------------------

def write_results(results: list[dict], output_file: Path | None = None) -> Path:
    RESULTS_DIR.mkdir(parents=True, exist_ok=True)

    if output_file is None:
        output_file = RESULTS_FILE

    run_count = max((len(result["times"]) for result in results), default=0)
    header = ["threads"]
    header.extend(f"run_{index}_ms" for index in range(1, run_count + 1))
    header.extend(["median_ms", "speedup"])

    with output_file.open("w", newline="") as file:
        writer = csv.writer(file)
        writer.writerow(header)

        for result in results:
            row = [result["threads"]]
            row.extend(f"{value:.3f}" for value in result["times"])
            row.extend([
                f'{result["median_ms"]:.3f}',
                f'{result["speedup"]:.3f}',
            ])
            writer.writerow(row)

    return output_file


def load_results(csv_file: Path) -> list[dict]:
    with csv_file.open(newline="") as file:
        reader = csv.DictReader(file)

        if reader.fieldnames is None:
            raise ValueError(f"CSV file is empty or missing a header: {csv_file}")

        run_fields = [
            name for name in reader.fieldnames
            if name.startswith("run_") and name.endswith("_ms")
        ]

        results = []

        for row in reader:
            threads = int(row["threads"])
            times = []
            for index in range(1, len(run_fields) + 1):
                field_name = f"run_{index}_ms"
                value = row.get(field_name, "")
                if value:
                    times.append(float(value))

            if not times:
                continue

            median_ms = float(row["median_ms"]) if row.get("median_ms") else statistics.median(times)
            speedup = float(row["speedup"]) if row.get("speedup") else 0.0

            results.append({
                "threads": threads,
                "times": times,
                "median_ms": median_ms,
                "speedup": speedup,
            })

    return results


# ---------------------------------------------------------------------------
# Plotting
# ---------------------------------------------------------------------------

def plot_execution_time(
    results: list[dict],
    output_dir: Path = RESULTS_DIR,
    y_limit: float | None = None,
) -> None:
    threads = [r["threads"] for r in results]
    times = [r["median_ms"] for r in results]

    plt.figure()

    plt.plot(threads, times, marker="o")

    plt.xlabel("Number of threads")
    plt.ylabel("Execution time (ms)")
    plt.title("Point generation execution time")

    plt.xticks(threads)
    if y_limit is not None:
        plt.ylim(0, y_limit)
    plt.grid(True)

    plt.tight_layout()

    output_file = output_dir / "execution_time.png"
    plt.savefig(output_file, dpi=200)
    plt.close()


def plot_speedup(
    results: list[dict],
    output_dir: Path = RESULTS_DIR,
    y_max: float | None = None,
) -> None:
    threads = [r["threads"] for r in results]
    speedups = [r["speedup"] for r in results]

    plt.figure()

    plt.plot(threads, speedups, marker="o", label="Measured speedup")
    plt.plot(threads, threads, linestyle="--", label="Ideal speedup")

    plt.xlabel("Number of threads")
    plt.ylabel("Speedup")
    plt.title("Point generation speedup")

    plt.xticks(threads)
    if y_max is None:
        y_max = 3.0
    plt.ylim(0.5, y_max)
    plt.grid(True)
    plt.legend()

    plt.tight_layout()

    output_file = output_dir / "speedup.png"
    plt.savefig(output_file, dpi=200)
    plt.close()


# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------

def main() -> None:
    parser = argparse.ArgumentParser(description="Benchmark point generation and plot results.")
    parser.add_argument(
        "--plot-only",
        action="store_true",
        help="Generate plots from the saved CSV without rerunning the benchmark.",
    )
    parser.add_argument(
        "--csv",
        type=Path,
        default=RESULTS_FILE,
        help="CSV file to read from for plotting. Defaults to benchmark/benchmark_results.csv.",
    )
    parser.add_argument(
        "--y-limit",
        type=float,
        default=None,
        help="Upper bound for the execution time plot in milliseconds.",
    )
    parser.add_argument(
        "--speedup-ymax",
        type=float,
        default=None,
        help="Upper bound for the speedup plot. Defaults to 3.5 when omitted.",
    )
    parser.add_argument(
        "--pause",
        type=float,
        default=PAUSE_SECONDS,
        help="Delay in seconds before each benchmark execution to reduce run-to-run jitter.",
    )
    args = parser.parse_args()

    if args.plot_only:
        if not args.csv.exists():
            raise FileNotFoundError(f"Results CSV not found: {args.csv}")

        results = load_results(args.csv)
        if not results:
            raise ValueError(f"No benchmark rows found in {args.csv}")

        RESULTS_DIR.mkdir(parents=True, exist_ok=True)
        plot_execution_time(results, RESULTS_DIR, y_limit=args.y_limit)
        plot_speedup(results, RESULTS_DIR, y_max=args.speedup_ymax)

        print(f"Plots generated from {args.csv}")
        return

    if not EXECUTABLE.exists():
        raise FileNotFoundError(f"Executable not found: {EXECUTABLE}")

    RESULTS_DIR.mkdir(parents=True, exist_ok=True)

    all_results = []

    for num_threads in THREAD_COUNTS:
        times = benchmark(num_threads, pause_seconds=args.pause)
        median_ms = statistics.median(times)

        all_results.append({
            "threads": num_threads,
            "times": times,
            "median_ms": median_ms,
        })

    baseline = all_results[0]["median_ms"]

    for result in all_results:
        result["speedup"] = baseline / result["median_ms"]

    csv_file = write_results(all_results, RESULTS_FILE)
    plotted_results = load_results(RESULTS_FILE)

    plot_execution_time(plotted_results, RESULTS_DIR, y_limit=args.y_limit)
    plot_speedup(plotted_results, RESULTS_DIR, y_max=args.speedup_ymax)

    print("\n----------------------------------------")
    print("Benchmark completed")
    print("----------------------------------------")

    print(f"Results: {RESULTS_FILE}")
    print(f"Plot:    {RESULTS_DIR / 'execution_time.png'}")
    print(f"Plot:    {RESULTS_DIR / 'speedup.png'}")

    print("\nSummary:")
    print(f"{'Threads':>8} {'Median (ms)':>15} {'Speedup':>12}")

    for result in plotted_results:
        print(
            f"{result['threads']:>8} "
            f"{result['median_ms']:>15.3f} "
            f"{result['speedup']:>12.3f}"
        )
        

if __name__ == "__main__":
    main()

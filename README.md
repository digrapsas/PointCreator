# PointCreator# PointCreator

A multithreaded C++ program for generating random points in 3D around a user-defined center.

The distance of each point from the center follows a normal distribution, while the direction is uniformly distributed over the sphere. The point generation step is parallelized using the C++ Standard Library and its execution time is measured.

## Overview

PointCreator generates a configurable number of random points around a 3D center point.

Each point is defined by:

* its **distance** from the center;
* its **direction** from the center.

The radial distance follows a normal distribution:

* `R_mean` defines the mean distance from the center;
* `R_dev` defines the standard deviation.

The direction is uniformly distributed over the surface of a sphere, ensuring that there is no preferred direction in 3D space.

This makes it possible to generate both spherical shells and volumetric point clouds depending on the selected radial distribution.

### Special cases

* `R_dev = 0`: all points lie on the surface of a sphere with radius `R_mean`;
* `R_mean = 0`: points are clustered around the center;
* `R_mean = 0` and `R_dev = 0` simultaneously is not allowed.

## Features

* Random 3D point generation
* Normally distributed radial distance
* Uniformly distributed 3D direction
* Configurable center point
* Configurable number of points
* Multithreaded point generation using the C++ Standard Library
* Execution-time measurement of the generation step
* Text output compatible with the specification
* Performance benchmarking and scalability analysis

## Input

The program takes the following parameters:

```text
sphere_points <number_of_threads> <number_of_points> <x_c> <y_c> <z_c> <R_mean> <R_dev>
```

Where:

| Parameter           | Description                                 |
| ------------------- | ------------------------------------------- |
| `number_of_threads` | Number of threads used for point generation |
| `number_of_points`  | Number of points to generate                |
| `(x_c,y_c,z_c)`     | (x,y,z) coordinates of the center           |
| `R_mean`            | Mean radial distance                        |
| `R_dev`             | Standard deviation of the radial distance   |

`R_mean` and `R_dev` cannot both be zero.

## Example

Generate 1000 points uniformly distributed on a sphere of radius 4 centered at the origin, using a single thread:

```bash
sphere_points 1 1000 0.0 0.0 0.0 4.0 0.0
```

The generated points are written to:

```text
points.txt
```

using the following format:

```text
<x_1> <y_1> <z_1>
<x_2> <y_2> <z_2>
...
```

Coordinates are separated by spaces.

## Point Generation

For each generated point, a radial distance is sampled from the specified normal distribution and a uniformly distributed direction is generated.

The resulting Cartesian coordinates are obtained by combining the sampled radius, direction and center position.

The point-generation stage is divided between multiple worker threads. Each thread generates an independent subset of the requested points.

## Multithreading

Point generation is parallelized using the C++ Standard Library threading facilities.

The workload is divided between the requested number of threads, with each thread responsible for generating its own portion of the point cloud.

Random-number generation is kept thread-local to avoid unnecessary synchronization between worker threads.

This design allows the computational workload to scale with the number of available CPU cores while avoiding a shared random-number generator.

## Performance

The point-generation stage is timed independently from file output in order to measure the computational part of the program.

Benchmarking is performed by keeping the problem size fixed and increasing the number of threads.

The following metrics can be used to evaluate scalability:

* **Execution time**
* **Speedup**
* **Parallel efficiency**

For a given number of threads `N`, speedup is measured relative to the single-threaded execution:

```text
Speedup(N) = T(1) / T(N)
```

where `T(N)` is the point-generation execution time using `N` threads.

The ideal speedup is linear:

```text
Speedup(N) = N
```

### Benchmark results

[Insert execution-time graph here]

[Insert speedup graph here]

Benchmark configuration:

* CPU: ...
* Compiler: ...
* Compiler version: ...
* Build type: Release
* Number of generated points: ...
* Operating system: ...

## Example Output

### Generated point cloud

[Insert 3D visualization here]

The visualization shows an example of the generated point distribution for a selected center, mean radius and standard deviation.

## Build

```bash
git clone <repository-url>
cd PointCreator

# Build instructions
...
```

A Release build is recommended when measuring performance.

## Project Structure

```text
.
├── src/
│   └── ...
├── tests/
│   └── ...
├── benchmarks/
│   └── ...
├── examples/
│   └── ...
├── CMakeLists.txt
└── README.md
```

## Testing

The project includes tests covering:

* command-line argument validation;
* invalid parameter combinations;
* point-generation edge cases;
* output format;
* statistical properties of generated points.

## Design Considerations

The implementation focuses on keeping the computational part independent from output and synchronization.

In particular:

* worker threads generate points independently;
* random-number generators are local to each thread;
* synchronization is minimized;
* file output is performed separately from the computational kernel;
* memory is allocated before parallel execution when possible.

These choices allow the benchmark to focus on the scalability of the point-generation algorithm rather than on file I/O.

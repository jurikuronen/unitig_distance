# unitig_distance
unitig_distance is a command line program that calculates graph-theoretic shortest-path distances in bulk for graphs which do not admit any straightforward decompositions. That is, it is a rather brute-force-ish distance calculator that aims to speed up the calculations by smart arrangement of the graph search jobs and through parallel operation.

As the name suggests, the impetus for designing unitig_distance comes from bioinformatics. The primary motivation is to be able to calculate, in reasonable time, millions of distance queries in compacted de Bruijn graphs constructed from genome references, where graph vertices correspond to unitigs. These kind of graphs tend to be very large (millions of vertices and edges) and highly connected with vertex-connectivity in the bulk of the graph being at least 4, which means that typical graph decompositions into 2-connected or 3-connected components in order to obtain fast distance calculation algorithms are not suitable.

unitig_distance can be used to supplement programs such as [SpydrPick](https://github.com/santeripuranen/SpydrPick) that calculate pairwise scores for the unitigs, but cannot calculate their distances in the underlying compacted de Bruijn graph. For such use cases, see [Input files - Distance queries file](#distance-queries-file) and [Usage - Calculating distances in compacted de Bruijn graphs](#calculating-distances-in-compacted-de-bruijn-graphs).

See also the project [gfa1_parser](https://github.com/jurikuronen/gfa1_parser) which can be used to create suitable input files for unitig_distance from genome references.

## Table of contents

- [Installation from source with a C++11 compliant compiler](#installation-from-source-with-a-c11-compliant-compiler)
- [Input files](#input-files)
  - [General graph](#general-graph)
  - [Compacted de Bruijn Graph](#compacted-de-bruijn-graph)
    - [Single genome graphs](#single-genome-graphs)
  - [Applying a filter on the graph](#applying-a-filter-on-the-graph)
  - [Distance queries file](#distance-queries-file)
- [Usage](#usage)
  - [Calculating distances in general graphs](#calculating-distances-in-general-graphs)
  - [Calculating distances in compacted de Bruijn graphs](#calculating-distances-in-compacted-de-bruijn-graphs)
  - [Calculating distances in single genome graphs](#calculating-distances-in-single-genome-graphs)

## Installation from source with a C++11 compliant compiler
```
git clone https://github.com/jurikuronen/unitig_distance
cd unitig_distance
make
```
This will create an executable named `unitig_distance` inside the `bin` directory.

## Input files
unitig_distance reads as input **space-separated values** text files whose paths and any additional options should be provided as command line arguments. This section details how the input files should be prepared and provided.

### General Graph
The simplest graph input file for unitig_distance is an edges file (`-E [ --edges-file ] arg`) where lines should have the format
```
v w (weight)
```
for vertices `v` and `w` connected by an edge with weight `weight`. The `weight` field is optional and defaults to the value 1.0. The vertices must be sequentially mapped starting from 0 or 1 (`-1g [ --graphs-one-based ]`).

### Compacted de Bruijn Graph
To construct a compacted de Bruijn graph, unitig_distance needs to know the value of the k-mer length `k` (`-k [ --k-mer-length ] arg`) and requires a separate unitigs file (`-U [ --unitigs-file ] arg`) where each line has the format
```
id sequence
```
where `sequence` is the unitig's sequence. The unitig's `id` is arbitrary, but nevertheless the unitigs should be in the correct order with respect to the vertex mapping. The field `id` is not used by unitig_distance, but is required to retain compatibility with other programs. The unitig `sequence` together with the k-mer length `k` will be used to determine self-edge weights.

The lines in the edges file (`-E [ --edges-file ] arg`) should have the **extended format**
```
v w edge_type (overlap)
```
where `v` and `w` correspond to distinct unitigs (according to the order in the unitigs file) which are connected according to the `edge_type` (FF, RR, FR or RF, indicating the overlap type of the forward/reverse complement sequences). The `overlap` field is optional and mostly used to distinguish between `k-1`-overlapping edges (default) and `0`-overlapping edges. Edges of the latter type are skipped in unitig_distance, since they often correspond to read errors in the genome references.

#### Single genome graphs
After providing the necessary files to construct a [compacted de Bruijn graph](#compacted-de-bruijn-graph), unitig_distance can also construct all the individual *single genome graphs* that compose the full graph. Each single genome graph requires a similar edges file as the full compacted de Bruijn graph. All such edge file paths should be collected in a single genome graph paths file (`-S [ --sgg-paths-file ] arg`) with one single genome graph edges file path per line. Distance calculation can be restricted to the single genome graphs (`-r [ --run-sggs-only]`).

### Applying a filter on the graph
It is possible to filter out unwanted vertices/unitigs (e.g. repetitive elements) by providing a filter file (`-F [ --filter-file ] arg`) where each line has the format
```
v (filter_value)
```
and a filter criterion (`-c [ --filter-criterion ] arg (=2.0)`). This will cause unitig_distance to construct an additional filtered graph, where given vertices `v` with `filter_value >= filter_criterion` will be disconnected in the graph. The `filter_value` field is optional and defauls to an infinite value.

### Distance queries file
The simplest format for the queries file (`-Q [ --queries-file ] arg`) is a file where each line has the format
```
v w
```
that is, each line is a distance query for vertices `v` and `w`. The vertices in the distance query must be sequentially mapped starting from 0 or 1 (`-1q [ --queries-one-based ]`). **Note: the distance queries file can use 1-based numbering for the vertices even if the graph files do not.**

If the distance queries file is the output of a program such as [SpydrPick](https://github.com/santeripuranen/SpydrPick) that has calculated the top pairwise scores for a set of unitig pairs, unitig_distance assumes that the line format is
```
v w distance <unused> score <unused> ... <unused>
```
where the third column `distance` will be replaced by unitig_distance, fourth column is unused, fifth `score` column will be used to calculate some statistics and the remaining columns are unused. The unused fields will still be written back to unitig_distance's output files &ndash; only the `distance` field will be replaced.

Restricting the number of queries to be read from the queries file can be done with `-n [ --n-queries ] arg (=inf)`.

## Usage
This section contains examples of how to use unitig_distance. 

A list of available options is also available with the command line argument `-h [ --help ]`.

### Calculating distances in general graphs
The following constructs a general graph from an edges file (`-E <path_to_edges_file>`) and calculates distances between 100 (`-n 100`) vertex pairs read from the queries file (`-Q <path_to_queries_file>`) in parallel using 4 threads (`-t 4`). The output will be written to `<output_stem>.ud_0_based`. Verbose-mode (`-v`) is set and a log will be written both to the terminal and to the file `<output_stem>.ud_log`.
```
./bin/unitig_distance -E <path_to_edges_file> \
                     -Q <path_to_queries_file> -n 100 \
                     -o <output_stem> -t 4 -v | tee <output_stem>.ud_log
```

### Calculating distances in compacted de Bruijn graphs
The following constructs a compacted de Bruijn graph from an edges file (`-E <path_to_edges_file>`) and a unitigs file (`-U <path_to_unitigs_file>`) with k-mer length 61 (`-k 61`) and calculates distances between all vertex pairs read from the queries file (`-Q <path_to_queries_file>`) in parallel using 16 threads (`-t 16`). The queries use one-based numbering (`-1q`). The output will be written to `<output_stem>.ud_0_based`. Verbose-mode (`-v`) is set and a log will be written both to the terminal and to the file `<output_stem>.ud_log`.
```
./bin/unitig_distance -E <path_to_edges_file> \
                     -U <path_to_unitigs_file> -k 61 \
                     -Q <path_to_queries_file> -1q \
                     -o <output_stem> -t 16 -v | tee <output_stem>.ud_log
```

### Calculating distances in single genome graphs
Following from the above section ([Calculating distances in compacted de Bruijn graphs](#calculating-distances-in-compacted-de-bruijn-graphs)), the following constructs all the single genome graphs in the single genome graph paths file (`-S [ --sgg-paths-file ] arg`) and calculates distances in these graphs only (`-r [ --run-sggs-only ]`).
```
./bin/unitig_distance -E <path_to_edges_file> \
                     -U <path_to_unitigs_file> -k 61 \
                     -Q <path_to_queries_file> -1q \
                     -S <path_to_sggs_file> -r \
                     -o <output_stem> -t 16 -v | tee <output_stem>.ud_log
```
The output will be written to
- `<output_stem>.ud_sgg_min_0_based`
- `<output_stem>.ud_sgg_max_0_based`
- `<output_stem>.ud_sgg_mean_0_based`
- `<output_stem>.ud_sgg_counts_0_based`

where the \*min\*/\*max\*/\*mean\* files contain minimum, maximum and mean distances across the single genome graphs and the \*counts\* file contains for each query the count of connected vertex pairs across the single genome graphs.

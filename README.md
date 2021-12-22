# unitig_distance
unitig_distance is a command line program that calculates graph-theoretic shortest-path distances in bulk for graphs which do not admit any straightforward decompositions. That is, it is a rather brute-force-ish distance calculator that aims to speed up the calculations by smart arrangement of the graph search jobs and through parallel operation.

As the name suggests, the impetus for designing unitig_distance comes from bioinformatics. The primary motivation is to be able to calculate, in reasonable time, millions of distance queries in compacted de Bruijn graphs constructed from genome references, where graph vertices correspond to unitigs. These kind of graphs tend to be very large (millions of vertices and edges) and highly connected with vertex-connectivity in the bulk of the graph being at least 4, which means that typical graph decompositions into 2-connected or 3-connected components in order to obtain fast distance calculation algorithms are not suitable.

unitig_distance can be used to supplement programs such as [SpydrPick](https://github.com/santeripuranen/SpydrPick) that calculate pairwise scores for the unitigs, but cannot calculate their distances in the underlying compacted de Bruijn graph. For such use cases, see [Input files - Distance queries file](#distance-queries-file), [Usage - Calculating distances in compacted de Bruijn graphs](#calculating-distances-in-compacted-de-bruijn-graphs) and [Usage - Determining outliers from supplied scores](#determining-outliers-from-supplied-scores).

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
  - [List of available options](#list-of-available-options)
  - [Calculating distances in general graphs](#calculating-distances-in-general-graphs)
  - [Calculating distances in compacted de Bruijn graphs](#calculating-distances-in-compacted-de-bruijn-graphs)
  - [Calculating distances in single genome graphs](#calculating-distances-in-single-genome-graphs)
  - [Determining outliers from supplied scores](#determining-outliers-from-supplied-scores)

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

### List of available options
This list is available with the command line argument `-h [ --help ]`.
```
Graph edges:                                  
  -E  [ --edges-file ] arg                    Path to file containing graph edges.
  -1g [ --graphs-one-based ]                  Graph files use one-based numbering.
                                              
Filter the graph:                             
  -F  [ --filter-file ] arg                   Path to file containing vertices/unitigs that will be filtered.
  -1f [ --filter-one-based ]                  Filter file uses one-based numbering.
  -c  [ --filter-criterion ] arg (=2.0)       Criterion for the filter.
                                              
CDBG operating mode:                          
  -U  [ --unitigs-file ] arg                  Path to file containing unitigs.
  -k  [ --k-mer-length ] arg                  k-mer length.
                                              
CDBG and/or SGGS operating mode:              
  -S  [ --sgg-paths-file ] arg                Path to file containing paths to single genome graph edge files.
  -r  [ --run-sggs-only ]                     Calculate distances only in the single genome graphs.
                                              
Distance queries:                             
  -Q  [ --queries-file ] arg                  Path to queries file.
  -1q [ --queries-one-based ]                 Queries file uses one-based numbering.
  -n  [ --n-queries ] arg (=inf)              Number of queries to read from the queries file.
  -b  [ --block-size ] arg (=10000)           Process this many queries/tasks at a time.
  -d  [ --max-distance ] arg (=inf)           Maximum allowed graph distance (for constraining the searches).
                                              
Tools for determining outliers:               
  -x  [ --output-outliers ]                   Output a list of outliers and outlier statistics.
  -C  [ --sgg-counts-file ] arg               Path to single genome graph counts file.
  -1c [ --sgg-counts-one-based ]              Single genome graph counts file uses one-based numbering.
  -Cc [ --sgg-count-threshold ] arg (=10)     Filter low count single genome graph distances.
  -l  [ --ld-distance ] arg (=-1)             Linkage disequilibrium distance (automatically determined if negative).
  -lm [ --ld-distance-min ] arg (=1000)       Minimum ld distance for automatic ld distance determination.
  -ls [ --ld-distance-score ] arg (=0.8)      Score difference threshold for automatic ld distance determination.
  -ln [ --ld-distance-nth-score ] arg (=10)   Use nth max score for automatic ld distance determination.
  -ot [ --outlier-threshold ] arg             Set outlier threshold to a custom value.
                                              
Other arguments.                              
  -o  [ --output-stem ] arg (=out)            Path for output files (without extension).
  -1o [ --output-one-based ]                  Output files use one-based numbering.
  -1  [ --all-one-based ]                     Use one-based numbering for everything.
  -t  [ --threads ] arg (=1)                  Number of threads.
  -v  [ --verbose ]                           Be verbose.
  -h  [ --help ]                              Print this list.
```

### Calculating distances in general graphs
The following constructs a general graph from an edges file (`-E <path_to_edges_file>`) and calculates distances between 100 (`-n 100`) vertex pairs read from the queries file (`-Q <path_to_queries_file>`) in parallel using 4 threads (`-t 4`). The output will be written to `<output_stem>.ud_0_based`. Verbose-mode (`-v`) is set and a log will be written both to the terminal and to the file `<output_stem>.ud_log`.
```
./bin/unitig_distance -E <path_to_edges_file> \
                      -Q <path_to_queries_file> -n 100 \
                      -o <output_stem> -t 4 -v | tee <output_stem>.ud_log
```

### Calculating distances in compacted de Bruijn graphs
The following constructs a compacted de Bruijn graph from an edges file (`-E <path_to_edges_file>`) and a unitigs file (`-U <path_to_unitigs_file>`) with k-mer length 61 (`-k 61`) and calculates distances between all vertex pairs read from the queries file (`-Q <path_to_queries_file>`) in parallel using 16 threads (`-t 16`). The queries use one-based numbering (`-1q`). Verbose-mode (`-v`) is set and a log will be written both to the terminal and to the file `<output_stem>.ud_log`.
```
./bin/unitig_distance -E <path_to_edges_file> \
                      -U <path_to_unitigs_file> -k 61 \
                      -Q <path_to_queries_file> -1q \
                      -o <output_stem> -t 16 -v | tee <output_stem>.ud_log
```
The output will be written to `<output_stem>.ud_0_based`.

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

where the \*min\*/\*max\*/\*mean\* files contain minimum, maximum and mean distances across the single genome graphs and the \*counts\* file contains, for each query, the count of single genome graphs where the query's vertex pair is connected.

### Determining outliers from supplied scores
When the queries contain pairwise scores for the unitigs, for example when the output of a program such as [SpydrPick](https://github.com/santeripuranen/SpydrPick) is provided as the distance queries file (see [Input files - Distance queries file](#distance-queries-file)), unitig_distance can automatically determine outliers and outlier stats for all graphs being worked on with the command line argument `-x [ --output-outliers ]`. When working with single genome graphs, vertex pairs in the queries that are connected in less than `sgg_count_threshold` (default: 10) single genome graphs will also be filtered out. This option can be modified with the command line argument `-Cc [ --sgg-count-threshold ] arg (=10)` with a value of 0 completely disabling it.

The outlier threshold estimation is currently experimental, intended to aid with post-processing the results and should not be depended on fully. The estimation only works well with sufficiently large inputs (ideally, the queries should cover all unitigs of interest). It is based on Tukey's outlier test, which assesses how extreme a score is compared to a global background distribution. As background distribution, an extreme value distribution is fitted from maximum unitig scores beyond a distance-based cutoff in order to deal with linkage disequilibrium. The linkage disequilibrium distance cutoff can be specified with the command line argument `-l [ --ld-distance ] arg (=-1)`, but unitig_distance can also attempt to determine it automatically if left at its default negative value (recommended). If the linkage disequilibrium distance cutoff has been set, it's also possible to set the outlier threshold to a custom value with the command line argument `-ot [ --outlier-threshold ] arg`, skipping Tukey's outlier test.

unitig_distance can run in outlier tools mode for already calculated distances by supplying the distances with the command line argument `-Q [ --queries-file ] arg`. In this mode, no graph files should be supplied, otherwise unitig_distance will simply recalculate the distances using the queries file as normal queries input. An example is provided below for how to use this mode.

**Outlier tools example.** Assume the distances have been calculated from a queries file that contained pairwise scores according to the example at [Calculating distances in single genome graphs](#calculating-distances-in-single-genome-graphs). The following runs unitig_distance in outlier tools mode (`-x`) using the mean distances as the queries file (`-Q <output_stem>.ud_sgg_mean_0_based`). The counts file (`-C <output_stem>.ud_sgg_counts_0_based`) is provided as well and the count threshold is set to 50 (`-Cc 50`). Unitig_distance will determine the linkage disequilibrium distance cutoff automatically and calculates the outlier threshold and extreme outlier threshold values, which will be written to the outlier stats file. Verbose-mode (`-v`) is set and a log will be written to the terminal.
```
./bin/unitig_distance -x --Q <output_stem>.ud_sgg_mean_0_based \
                      -C <output_stem>.ud_sgg_counts_0_based -Cc 50 \
                      -o <output_stem_sgg> -v
```
The linkage disequilibrium distance cutoff, outlier threshold, extreme outlier threshold and `sgg_count_threshold` values will be written to `<output_stem_sgg>.ud_outlier_stats`. Then, these values will be used to collect the queries which will be written to `<output_stem_sgg>.ud_outliers_0_based`. Notice the updated output name so that the file `<output_stem>.ud_outliers_0_based` won't be overwritten.

**Plotting the results.** It is recommended to check how the results look like graphically by visualizing the results with the provided R script at [unitig_distance/scripts/](scripts). Afterwards, it is easy to rerun unitig_distance in outlier tools mode with updated parameter values if necessary.


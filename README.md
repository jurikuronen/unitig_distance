# unitig_distance
Answer distance queries for unitig graphs efficiently.

## Installation
Simply type `make` in the working directory.

## Run
Basic use with default options:
```
./bin/ud -V <path_to_nodes> -E <path_to_edges> -k <kmer_length> -C <path_to_couplings>
```
Use `-1` if couplings are one-based. Graph nodes/edges one-basedness will be detected by the Graph constructor.

Print a list of available options with:
```
./bin/ud -h
```

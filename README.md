# unitig_distance
Answer distance queries for unitig graphs efficiently.

## Installation
```
mkdir build bin
make
```

## Run
Basic use with default options:
```
./bin/ud -V <path_to_nodes> -E <path_to_edges> -C <path_to_couplings>
```
Use `-1` if couplings are one-based. Graph nodes/edges one-basedness will be detected by the Graph constructor.

List of available options with:
```
./bin/ud -h
```

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
./bin/ud -v <path_to_nodes> -e <path_to_edges> -c <path_to_couplings>
```
Use `-1` if couplings are one-based. Graph nodes/edges one-basedness will be detected by the Graph constructor.

List of available options with:
```
./bin/ud -h
```

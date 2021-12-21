# WFPOA

An extension to the [wavefront algorithm](https://doi.org/10.1093/bioinformatics/btaa777) that generalizes it to work on
directed acyclic sequence graphs.

This follows the basic formulation of [partial order alignment](https://doi.org/10.1093/bioinformatics/18.3.452),
wherein the recurrence relations defining alignment are extended to consider the topology of a target graph.

To build the project:

```shell
cmake -H. -Bbuild && cmake --build build -- -j 16
```

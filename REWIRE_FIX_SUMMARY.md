# Fix for rewire_degseq Bug

## Problem
The `rewire_degseq` function in `randgraph.hpp` was incorrectly swapping edge **weights** instead of actually **rewiring** the network connections. This meant that:
- The network structure (who connects to whom) remained unchanged
- Only the weight values stored in the adjacency maps were being swapped

## Root Cause
In the AdjList version:
```cpp
// OLD CODE (incorrect):
std::swap(p0[id01], p1[id11]);  // This only swaps the weights!
```

The code was swapping the VALUES in the map (the weights), not the KEYS (the neighbor IDs).

## Solution
The fix properly rewires edges by:
1. Saving the edge weights
2. Removing the old edges from the adjacency lists
3. Adding new edges with the swapped connections
4. For undirected graphs, also updating the reverse edges
5. Checking for duplicate edges before swapping to preserve the degree sequence

### AdjList Version Changes
- Replace `std::swap(p0[id01], p1[id11])` with proper edge removal and addition
- Add duplicate edge detection to skip rewires that would create parallel edges
- Ensure both directions are updated for undirected graphs

### Agent Vector Version Changes
- Fixed bug where `model->get_agents()[id0]` should have been `model->get_agents()[non_isolates[id0]]`
- Added duplicate edge detection before calling `swap_neighbors()`
- The `swap_neighbors()` function itself was already correctly implemented

## Testing
A standalone test (`test-rewire-simple.cpp`) was created to verify:
1. **Degree preservation**: Each node maintains its original number of connections
2. **Network structure change**: The rewiring actually changes who connects to whom
3. **Graph symmetry**: For undirected graphs, if A connects to B, then B connects to A

### Test Results
```
=== Testing rewire_degseq fix ===
Degrees preserved: YES
Nodes with changed neighbors: 15 out of 20
Graph symmetric: YES

PASS: All tests successful!
```

## Files Modified
- `include/epiworld/randgraph.hpp`: Fixed both `rewire_degseq` implementations
- `tests/test-rewire-simple.cpp`: Standalone test to verify the fix

## Notes
- The main test suite has pre-existing compilation errors in unrelated models that prevent it from compiling
- The standalone test validates the core fix without requiring the full test infrastructure
- The fix is minimal and surgical, changing only what's necessary to correct the rewiring logic

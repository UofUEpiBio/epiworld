## Example: `17-sbm-scalability`

Scalability benchmark comparing the wall-clock time of three network
generators — **SBM**, **Bernoulli**, and **Small-world (Watts-Strogatz)** —
all producing undirected graphs with an expected degree of ~10.

**Parameters:**
- **SBM:** groups ∈ {3, 5, 10, 50}, total agents ∈ {1K, 10K, 100K},
  within-block degree 6, between-block degree 4.
- **Bernoulli:** p = 10/(n−1), total agents ∈ {1K, 10K, 100K}.
- **Small-world:** k = 10, p_rewire = 0.1, total agents ∈ {1K, 10K, 100K}.
- Repetitions per configuration: 10

Both the SBM and Bernoulli generators use the Batagelj-Brandes
geometric skipping algorithm (O(n + m) expected time).
The small-world generator builds a ring lattice and rewires edges.

The edge counts track the expected value of ~n_total × 10 / 2
(each of the ~10 contacts is undirected → 5 edges per agent).
Generation time for SBM and Bernoulli scales linearly with the
number of edges, consistent with O(n + m) complexity.

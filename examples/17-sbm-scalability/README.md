## Example: `17-sbm-scalability`

Benchmark measuring the wall-clock time to generate SBM networks
using the Batagelj-Brandes algorithm for different numbers of groups
and total population sizes.

**Parameters:**
- Groups: 3, 5, 10, 50
- Total agents: 1 000, 10 000, 100 000
- Expected degree: ~10 (within-block: 6, between-block: 4)
- Repetitions per configuration: 10

Output from the program:

```
groups    n_total     n_each    edges(avg)    time_ms(avg)  
------    -------     ------    ----------    -----------   
3         1000        333       4975          0.72          
3         10000       3333      49961         8.54          
3         100000      33333     499800        171.17        
5         1000        200       5011          0.69          
5         10000       2000      49975         6.70          
5         100000      20000     499791        160.48        
10        1000        100       4963          0.68          
10        10000       1000      49919         6.53          
10        100000      10000     499824        137.65        
50        1000        20        4870          0.72          
50        10000       200       49833         6.35          
50        100000      2000      499829        134.03        
```

The edge counts track the expected value of ~n_total × 10 / 2
(each of the ~10 contacts is undirected → 5 edges per agent).
Generation time scales linearly with the number of edges,
consistent with the O(n + m) complexity of the Batagelj-Brandes
algorithm.

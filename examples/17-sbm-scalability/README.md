## Example: `17-sbm-scalability`

Output from the program:

```
=== SBM (Batagelj-Brandes, expected degree ~10) ===
groups    n_total     n_each    edges(avg)    time_ms(avg)  
------    -------     ------    ----------    -----------   
3         1000        333       4975          0.69          
3         10000       3333      49961         9.15          
3         100000      33333     499800        215.28        
5         1000        200       5011          0.50          
5         10000       2000      49975         5.95          
5         100000      20000     499791        138.75        
10        1000        100       4963          0.41          
10        10000       1000      49919         4.25          
10        100000      10000     499824        63.02         
50        1000        20        4870          0.44          
50        10000       200       49833         3.98          
50        100000      2000      499829        68.85         

=== Bernoulli (Batagelj-Brandes, expected degree ~10) ===
n_total     edges(avg)    time_ms(avg)  
-------     ----------    -----------   
1000        4999          0.43          
10000       49858         4.33          
100000      499699        141.26        

=== Small-world (Watts-Strogatz, k=10, p_rewire=0.1) ===
n_total     edges(avg)    time_ms(avg)  
-------     ----------    -----------   
1000        5000          0.44          
10000       50000         11.20         
100000      500000        798.77        
```

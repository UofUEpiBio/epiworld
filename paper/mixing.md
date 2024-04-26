# Mixing probabilities in connected model
George G. Vega Yon, Ph.D.
2024-04-25

## Case 1: No grouping

We will look into the probability of drawing infected individuals to
simplify the algorithm. There are $I$ infected individuals at any time
in the simulation; thus, instead of drawing from $Bern(c/N, N)$, we will
be drawing from $Bern(c/N, I)$. The next step is to check which infected
individuals should be drawn. Let’s compare the distributions using the
hypergeometric as an example:

``` r
set.seed(132)
nsims <- 1e5
N <- 400
rate <- 2
p <- rate/N
I <- 10

sim_complex <- parallel::mclapply(1:nsims, \(i) {
  nsamples <- rbinom(N, N, p)
  sum(rhyper(N, m = I, n = N, k = nsamples) > 0)
}, mc.cores = 4L) |> unlist()

sim_simple <- parallel::mclapply(1:nsims, \(i) {
  sum(rbinom(N, I, p) > 0) 
}, mc.cores = 4L) |> unlist()


op <- par(mfrow = c(1,2))
MASS::truehist(sim_complex)
MASS::truehist(sim_simple)
```

![](mixing_files/figure-commonmark/Simulation-1.png)

``` r
par(op)

quantile(sim_complex)
```

      0%  25%  50%  75% 100% 
       3   16   19   22   40 

``` r
quantile(sim_simple)
```

      0%  25%  50%  75% 100% 
       4   17   19   22   40 

These two approaches are equivalent, but the second one is more
efficient from the computational perspective.

## Case 2: Grouping

This explores the case when we have mixing across groups. The question
is if we can replicate the effect at the group level.

``` r
set.seed(123133)

ngroups <- 3
mixing <- matrix(
  c(0.1, 0.2, 0.3, 0.2, 0.1, 0.2, 0.3, 0.2, 0.1),
  nrow = ngroups,
  ncol = ngroups
  )

mixing <- mixing/rowSums(mixing)
mixing
```

              [,1]      [,2]      [,3]
    [1,] 0.1666667 0.3333333 0.5000000
    [2,] 0.4000000 0.2000000 0.4000000
    [3,] 0.5000000 0.3333333 0.1666667

``` r
N <- 500
sizes <- c(100, 50, 350)
rate <- 1
p <- rate/N
I <- c(10, 5, 20)

ids <- rep.int(1:ngroups, times = sizes)

nsims <- 2e5

sim_complex <- parallel::mclapply(1:nsims, \(i) {

  # Sampling group first
  where_from <- lapply(1:ngroups, \(g) {
    sample(3, sizes[g], replace = TRUE, prob = mixing[g,])
  }) |> unlist() |> c()

  nsamples <- rbinom(N, sizes[where_from], p)
  sum(rhyper(N, m = I[where_from], n = sizes[where_from], k = nsamples) > 0)
}, mc.cores = 4L) |> unlist()
```

Using the alternative method in which we directly weight the
probabilities:

``` r
sim_simple <- parallel::mclapply(1:nsims, \(i) {
  where_from <- lapply(1:ngroups, \(g) {
    sample(3, sizes[g], replace = TRUE, prob = mixing[g,])
  }) |> unlist() |> c()

  sum(rbinom(N, I[where_from], p) > 0)

}, mc.cores = 4L) |> unlist()

op <- par(mfrow = c(1,2))
MASS::truehist(sim_complex)
MASS::truehist(sim_simple)
```

![](mixing_files/figure-commonmark/unnamed-chunk-3-1.png)

``` r
par(op)

quantile(sim_complex)
```

      0%  25%  50%  75% 100% 
       0    8   10   12   27 

``` r
quantile(sim_simple)
```

      0%  25%  50%  75% 100% 
       0    9   11   13   29 

``` r
(table(sim_complex) |> prop.table() |> cumsum() -
  table(sim_simple) |> prop.table() |> cumsum()) |> plot()
```

    Warning in cumsum(prop.table(table(sim_complex))) -
    cumsum(prop.table(table(sim_simple))): longer object length is not a multiple
    of shorter object length

![](mixing_files/figure-commonmark/unnamed-chunk-3-2.png)

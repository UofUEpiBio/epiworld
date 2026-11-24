## Overview

`epiworld::math` holds the plain functions of numbers that the rest of the library uses: they need no model, so any part of the library, and user code, can call them.

| Function | Value | Used by |
|---|---|---|
| `math::log1m(p)` | $\log(1 - p)$, accurate for every $p \le 1$ | the hazard of an exposure, $-\log(1-p)$; random graphs; geometric stage durations; `logit()` |
| `math::logistic(x)` | $1 / (1 + e^{-x})$ | tools and global events with logit-linear probabilities, `ModelDiffNet`, `ModelSIRLogit` |
| `math::logit(p)` | $\log(p / (1 - p))$ | `ModelDiffNet` |
| `math::geometric_skip(u, log_q)` | $\lfloor \log u / \log q \rfloor$ | random graphs (Batagelj and Brandes, 2005), persistence in mixing layers, geometric stage durations |
| `math::log_factorial(n)`, `math::dpois()`, `math::dgenint()`, `math::gen_int_mean()` | the distributions of [Random Number Generation](random-number-generation.md) | generation intervals |

The rest of this page derives `log1m()`, which epiworld implements itself instead of calling `std::log1p(-p)`.

## Why epiworld computes its own logarithm

Every infectious contact of a susceptible agent needs its hazard $\lambda = -\log(1 - p)$ (see [Combining Exposures](combining-exposures.md)). The standard library's `log1p` is accurate, but:

- **It is a library call** of about 20 cycles of throughput and more of latency, made once per contact. The model already reuses the last hazard when consecutive contacts share their probability (`Model::exposure_hazard()`); `log1m()` makes the remaining calls cheaper.
- **Its last bits differ between platforms.** glibc, Apple's libm, musl and Microsoft's runtime round `log1p` differently (Gladman et al.), so the same seed can give slightly different runs on different systems. `log1m()` uses only $+$, $-$, $\times$, $\div$ and `std::frexp`, which IEEE 754 defines exactly, so it gives the same bits everywhere, provided the compiler does not fuse multiplications and additions (see [Reproducibility](#reproducibility)).
- **`std::log(1 - p)` is not an option**: for small $p$, $1 - p$ rounds off the low digits of $p$. At $p = 10^{-9}$ that is a relative error of about $10^{-7}$ in the result.

## The method

Write $1 - p$ as a ratio around 1:

$$
1 - p = \frac{1 - s}{1 + s}
\quad\Longleftrightarrow\quad
s = \frac{p}{2 - p},
$$

so that

$$
\log(1 - p) = \log\frac{1 - s}{1 + s} = -2\,\operatorname{atanh}(s) = -2\left(s + \frac{s^3}{3} + \frac{s^5}{5} + \frac{s^7}{7} + \cdots\right).
$$

The series has only odd powers, so each term costs one multiplication by $z = s^2$, and it converges fast when $|s|$ is small. Two cases keep $|s| \le 3 - 2\sqrt{2} \approx 0.1716$:

1. **$1 - \sqrt{2} \le p \le 1 - \sqrt{2}/2$** (that is, $-0.414 \le p \le 0.293$, which covers almost every transmission probability): use $s = p/(2-p)$ directly. Computing $s$ has no cancellation, so the relative error is the same for $p = 10^{-300}$ as for $p = 0.2$. (The code computes $2s = 2p/(2-p)$, which is the same number doubled exactly, so that a tiny $p$ (subnormal, or normal below about $2^{-1021}$) does not underflow: $\log(1 - p) = -p$ there.)
2. **Otherwise**: compute $x = 1 - p$ (exact for $p \ge 1/2$, by Sterbenz's lemma), split it as $x = m\,2^e$ with $m \in [\sqrt{2}/2, \sqrt{2})$ (`std::frexp`, then one doubling), and use
   $$\log x = e \log 2 + \log m, \qquad \log m = 2 \operatorname{atanh}\!\left(\frac{m - 1}{m + 1}\right),$$
   with the same bound on $|s|$. $\log 2$ is stored in two parts, $\log 2 = h + l$, where $h$ has its last bits zero, so that $e\,h$ is exact for every exponent $e$ and $e\,l$ is added as a correction. This is the range reduction of fdlibm's `log1p` (Sun Microsystems, 1993), which glibc still uses.

The series is cut after the $s^{17}/17$ term. Evaluated in Horner form:

$$
2\operatorname{atanh}(s) \approx 2s\left(1 + z\left(\tfrac{1}{3} + z\left(\tfrac{1}{5} + \cdots + z\left(\tfrac{1}{15} + \tfrac{z}{17}\right)\cdots\right)\right)\right), \qquad z = s^2 .
$$

## Accuracy

The first omitted term bounds the truncation error, relative to the result:

$$
\frac{s^{18}}{19} \le \frac{0.1716^{18}}{19} \approx 9 \times 10^{-16}.
$$

| Terms up to | Truncation error (relative) |
|---|---|
| $s^9/9$ | $2 \times 10^{-9}$ |
| $s^{13}/13$ | $1 \times 10^{-12}$ |
| $s^{17}/17$ (epiworld) | $9 \times 10^{-16}$ |

`tests/61a-math-functions.cpp` compares `log1m(p)` with `std::log1p(-p)` for $p$ from $10^{-300}$ to $1 - 10^{-16}$, for negative $p$, and on both sides of the two branch boundaries. The largest relative difference is $1.1 \times 10^{-15}$, about five units in the last place of a double. Transmission probabilities are stored in `epiworld_double`, a `float` by default (relative precision $6 \times 10^{-8}$), so the hazards are exact to the precision of their inputs.

## Speed

CPU time per call, median of seven passes over $2^{20}$ values, g++ -O2, ARM64 (the epiworld-dev image on an Apple M-series machine):

| Inputs | `-std::log1p(-p)` | `-math::log1m(p)` | with `-ffp-contract=off` |
|---|---|---|---|
| $p$ log-uniform in $[10^{-4}, 0.25]$ | 2.78 ns | 1.60 ns | 2.19 ns (`log1p`: 2.70 ns) |
| $p$ uniform in $[0.3, 0.999]$ | 7.03 ns | 6.84 ns | 7.13 ns (`log1p`: 7.25 ns) |

The common case, small $p$, is about 1.7 times as fast; the reduced branch costs about the same as the library. With the hazard cache, most contacts need no logarithm at all; `log1m()` makes the rest cheaper and identical across platforms.

## Reproducibility

`log1m()` is a fixed sequence of IEEE 754 operations, each correctly rounded, so it returns the same bits on every platform, with one exception: a compiler may fuse $a \times b + c$ into one fused multiply-add (FMA), which rounds once instead of twice. GCC does this by default (`-ffp-contract=fast`), and ARM64 always has FMA instructions. The difference is in the last bit. Clang honors the `#pragma STDC FP_CONTRACT OFF` inside `log1m()`; with GCC, build with `-ffp-contract=off` for bit-identical results across platforms.

## Names

`log1m` follows the Stan Math Library, where `log1m(x)` is $\log(1 - x)$; `log1p(x)` is $\log(1 + x)$ in C and C++.

## References

- Batagelj, V., and Brandes, U. (2005). Efficient generation of large random networks. *Physical Review E*, 71, 036113. Geometric skipping for Bernoulli random graphs.
- Gladman, B., Innocente, V., Mather, J., and Zimmermann, P. *Accuracy of Mathematical Functions in Single, Double, Double Extended, and Quadruple Precision.* [members.loria.fr/PZimmermann/papers/accuracy.pdf](https://members.loria.fr/PZimmermann/papers/accuracy.pdf). How far each platform's `log1p` is from correctly rounded.
- Muller, J.-M. (2016). *Elementary Functions: Algorithms and Implementation* (3rd ed.). Birkhäuser. Range reduction and polynomial evaluation of elementary functions.
- Sun Microsystems (1993). fdlibm, `s_log1p.c`. [netlib.org/fdlibm/s_log1p.c](https://www.netlib.org/fdlibm/s_log1p.c). The reduction $1 + f = 2^k(1+f')$, the series in $s = f/(2+f)$, and the split of $\log 2$.
- Tang, P. T. P. (1990). Table-driven implementation of the logarithm function in IEEE floating-point arithmetic. *ACM Transactions on Mathematical Software*, 16(4), 378–400. [doi:10.1145/98267.98294](https://doi.org/10.1145/98267.98294). The basis of glibc's current `log`.

## See Also

- [Combining Exposures](combining-exposures.md) — why every exposure needs its hazard.
- [Push and Pull Transmission](transmission-sampling.md) — where the hazards are summed.
- [Random Number Generation](random-number-generation.md) — the distributions in `epiworld::math`.

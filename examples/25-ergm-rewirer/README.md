# A network layer that forms and dissolves ties (STERGM)

A rewiring function (`layer::Network::set_rewiring()`) that makes a network
layer change every day as a separable temporal ERGM (STERGM; Krivitsky and
Handcock 2014) does, with given coefficients. Estimating the coefficients is a
job for R's `ergm`/`tergm`; this example takes them as model parameters.

Each day, from yesterday's ties:

- **Formation**: a few Metropolis-Hastings toggles on the dyads not tied
  yesterday, under an ERGM with `edges`, `nodematch("group")` and `concurrent`
  (agents with two or more partners), evaluated on the formation network
  (yesterday's ties plus today's new ones). The parameters are
  `"Formation edges"`, `"Formation match"` and `"Formation concurrent"`.
- **Dissolution**: each of yesterday's ties ends with probability `1 / D`,
  `D` being `"Tie duration"` in days (`tergm`'s dissolution `edges`
  coefficient is `log(D - 1)`).

The rewirer is a plain function in `main.cpp`, `stergm(group, proposals)`:
other terms are other change statistics in the acceptance ratio.

The program runs an SIS infection on a static network and on the STERGM one,
both starting from the same random network, and prints the mean degree, the
share of ties within a group, the share of agents with concurrent partners and
the prevalence every 50 days. On the STERGM network, ties within a group rise
from about half to about 80% and concurrency falls, as the coefficients ask.

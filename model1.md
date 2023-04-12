 The dynamics of the simulation process are:

1. Discrete Markov process.

2. The simulation has the following parameters:

    a. New variant emergence at rate `X.`
    b. For each variant `k`:
    
    - Unvaccinated individuals become sick rate `C(k),`
    - Mortality rate `D(k),`
    - Recovery rate `H(k),`
    - Vaccines have an efficaccy rate `E(v,k)` and pseudo vaccines (recovered) have efficacy rate `E(r,k) < E(v,k).` In general, the probability of `i` acquiring the disease `k` from `j` will be equal to

    ```
    P(i gets the disease from j | their states) = C(k) * (1 - E(i,k)) * (1 - E(j, k))
    ```

    where `(i,j) in (u,v,r).` Efficacy rate for unvaccinated is zero.

    - Vaccinated individuals have a reduced mortality rate `D(k,v) > D(k),` and recovered individuals `D(k,r) in (D(k,v), D(k)]`
    - Vaccinated individuals have an increased recovery rate `H(k,v) > H(k)`, whereas recovered's rate `H(k,r) in [H(k), H(k,v)).`

    The sum of mortality and recovery rates is less than one since the difference represents no change.

    c. Each country vaccinates citizens at rate `V` function of `A` (availability) and `B` (citizens' acceptance rate.)
    d. In each country `i,` the entire population `N(i)` distributes between the following states:
    
      - Healthy unvaccinated (`N(i,t,u)`),
      - Healthy vaccinated (`N(i,t,v)`), 
      - Deceased (`N(i,t,d)`),
      - Recovered (`N(i,t,r)`),
      - Unvaccinated and sick with variant (`N(i,t,s,k|u)`) `k.`, and
      - Vaccinated and sick with variant (`N(i,t,s,k|v)`) `k.`

      Total sick are `N(i,t,k,s) = sum(g in {u,v}) N(i,t,k,s|g)`

    Globally, we keep track of the prevalence of new variants. Variants can disappear if no more individuals port the variant, i.e., the prevalence rate `P(k,t)=sum(i) N(i,s,k)` equals zero.

    d. Vaccines are manufactured at each country at rates `M(i)` and uniformly shared with other countries at rate `S(i).`
    c. Population flows between each country pair `(i,j)` at a rate `F(i,j).` Flows between countries do not change Population and are symmetric.

3. The simulation process is as follows:

    1. Countries are initialized with a total population `N(i).`
    2. Variant zero initializes at a random location `i,` with an initial prevalence `P(k,t) = N(i,t,k).`
    3. For time `t` in `(0,T)` do:

        a. Unvaccinated individuals can become sick of variant `k` with probability:

        ```
        Pr(h->s|i,t,k,u) ~ sum(g in {u,v}) (N(i,t - 1,s,k|g) + sum(j != i) F(i,j) * N(j,t-1,s,k|g)) * C(k) / (N(i) + sum(j != i) N(j))
        ```

        b. Vaccinated individuals can become sick of variant `k` with probability: `Pr(v->s|i,t,k,v) ~ Pr(h->s|i,t,k) * (1 - E(v,k)).`

        b. Recovered individuals can become sick of variant `k` with probability: `Pr(v->s|i,t,k,r) ~ Pr(h->s|i,t,k) * (1 - E(r,k)).`

        c. Sick individuals with variant `k` die with probability `D(k)` or recover with probability `H(k)`, otherwise they stay infected; with the rates depending on their vaccination status `v` or `n.`

        d. Unvaccinated individuals vaccinate in country `i` with probability `P(u->v) ~ V(A(i,t), B(i)).`

        e. The country vaccine supply changes.

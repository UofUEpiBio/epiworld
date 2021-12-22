
<!-- README.md is generated from README.Rmd. Please edit that file -->

# epiworld

<!-- badges: start -->
<!-- badges: end -->

The goal of epiworld is to …

## Installation

You can install the development version of epiworld from
[GitHub](https://github.com/) with:

``` r
# install.packages("devtools")
devtools::install_github("gvegayon/world-epi")
```

## Example

This is a basic example which shows you how to solve a common problem:

``` r
library(epiworld)
library(igraph)
#> 
#> Attaching package: 'igraph'
#> The following objects are masked from 'package:stats':
#> 
#>     decompose, spectrum
#> The following object is masked from 'package:base':
#> 
#>     union

set.seed(1231)
net <- sample_smallworld(1, 1000, 4, .1)

m <- new_epi_model()
geneseq <- c(T, F, T, T, F)

add_tool_immune(m, !geneseq, 1)
add_tool_vaccine(m, !geneseq, .5)
add_tool_mask(m, !geneseq, .25)

add_virus_covid19(m, geneseq, .5)

init_epi_model(m, 10, 10)
#> _________________________________________________________________________
print(m)
run_epi_model(m)
#> Running the model...
#> ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||| done.
```

## Code of Conduct

Please note that the epiworld project is released with a [Contributor
Code of
Conduct](https://contributor-covenant.org/version/2/0/CODE_OF_CONDUCT.html).
By contributing to this project, you agree to abide by its terms.

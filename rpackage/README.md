
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

m <- new_epi_model()
geneseq <- c(T, F, T, T, F)

add_tool_immune(m, !geneseq, 1)
add_tool_vaccine(m, !geneseq, .5)
add_tool_mask(m, !geneseq, .25)

add_virus_covid19(m, geneseq, .5)

init_epi_model(m, 10, 10)
#> _________________________________________________________________________

run_epi_model(m)
#> Running the model...
#> ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||| done.
```

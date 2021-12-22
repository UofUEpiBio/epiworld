library(epiworld)

m <- new_epi_model()
geneseq <- c(T, F, T, T, F)

add_tool_immune(m, !geneseq, 1)
add_tool_vaccine(m, !geneseq, .5)
add_tool_mask(m, !geneseq, .25)

add_virus_covid19(m, geneseq, .5)

init_epi_model(m, 10, 10)

run_epi_model(m)

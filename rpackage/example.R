library(epiworld)

m <- new_model()
geneseq <- c(T, F, T, T, F)

add_tool_immune(m, !geneseq, 1)
add_tool_vaccine(m, !geneseq, .5)
# (m, !geneseq, .5)

add_virus_covid19(m, geneseq, .5)


run_model(m, 10, 10)

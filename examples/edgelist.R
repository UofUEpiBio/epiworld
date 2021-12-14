library(igraph)

n <- 1000
set.seed(31)
g <- sample_smallworld(1, n, 4, 0.5)
write_graph(g, file = "examples/edgelist.txt", format = "edgelist")

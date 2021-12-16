library(igraph)

n <- 10000
set.seed(31)
g <- sample_smallworld(1, n, 10, 0.1)
write_graph(g, file = "examples/edgelist.txt", format = "edgelist")

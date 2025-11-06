library(igraph)

n <- 1000
set.seed(31)
g <- sample_smallworld(1, n, 5, 0.05)
write_graph(g, file = "examples/edgelist.txt", format = "edgelist")

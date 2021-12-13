totals   <- read.table("examples/total.txt", sep = " ", header = TRUE)
variants <- read.table("examples/variants.txt", sep = " ", header = TRUE)



plot(totals$date, totals$ninfected, type="l")
lines(totals$date, totals$nrecovered, lty = 2)

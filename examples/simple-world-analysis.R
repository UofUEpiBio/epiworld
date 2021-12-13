totals   <- read.table("examples/total.txt", sep = " ", header = TRUE)
variants <- read.table("examples/variants.txt", sep = " ", header = TRUE)


dat <- rbind(
  data.frame(
    date   = totals$date,
    status = "healthy",
    n      = totals$nhealthy
  ),
  data.frame(
    date   = totals$date,
    status = "infected",
    n      = totals$ninfected
  ),
  data.frame(
    date   = totals$date,
    status = "deceased",
    n      = totals$ndeceased
  )
)

library(ggplot2)

ggplot(dat, aes(x = date, y = n)) +
  geom_line(aes(color = status))

ggplot(totals, aes(x = date, y = nvariants)) +
  geom_line()

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

# Trajectory of infections -----------------------------------------------------
library(ggplot2)

dat |> #subset(date < 100) |>
ggplot(aes(x = date, y = n / 10000 * 100)) +
  geom_line(aes(color = status))

ggplot(totals, aes(x = date, y = nvariants)) +
  geom_line()

# Looking at the top 10 varians
top_variants <- by(variants$ninfected, INDICES = variants$id, FUN = max)
top_variants <- tail(sort(top_variants), 20)

variants |> subset(ninfected > 0) |>
  subset(id %in% as.integer(names(top_variants))) |>
  ggplot(aes(x = date, y = as.integer(ninfected), color = as.factor(id))) +
  # geom_smooth(aes(color = as.factor(id))) +
  geom_line()
  labs(y = "N infected", color = "Variant id")

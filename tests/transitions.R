s <- c("S", "E", "I", "R")
empty  <- matrix(0, 4, 4, dimnames =list(s,s))

# Good example, zero-sum transitions
queue_ego   <- empty
queue_alter <- empty
transm      <- empty

# Self queue affects only ego
queue_ego[1,2] <- 1
queue_ego[2,4] <- -1
queue_ego[3,4] <- -1
queue_ego

# Others are only affected when ego is infected
queue_alter["E", "I"] <- 1
queue_alter["I", "R"] <- -1
queue_alter

# Transmission is same for all
transm[c(1,5,6,10,11,14:16)] <- 1
transm

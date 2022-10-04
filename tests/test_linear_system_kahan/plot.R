#!/usr/bin/env Rscript

require(ggplot2)

data = read.table("x0.dat", header=T)
m = mean(data$X0)
s = sd(data$X0)
print(m)
print(s)
print(-log2(s/m))
data$X0 = data$X0/m - 1
print(summary(data))



p <- ggplot(data, aes(x=X0)) + geom_histogram(bins=35, color='grey35') + xlab(expression(X[0] / hat(mu)[0]-1)) + ylab("count")

p <- p + geom_segment(x = -s/2, y = 500, xend = +s/2, yend = 500,
               arrow = arrow(length = unit(0.03, "npc"), ends = "both", ), color="white")

p <- p + annotate("text", x=0, y = 540, parse = TRUE, size=8,
                       label= "hat(sigma) / abs(hat(mu))", color="white")

p <- p + theme_bw()
p



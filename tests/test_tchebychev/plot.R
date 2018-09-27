#!/usr/bin/env Rscript

require(ggplot2)

data = read.table("factor.dat", header=T)
print(summary(data))
ggplot(data, aes(x=z, y=y)) + geom_point(size=1) + theme_bw()

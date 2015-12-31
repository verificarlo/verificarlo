#!/usr/bin/env Rscript
require(ggplot2)
require(data.table)

mpfr = read.table("out_mpfr", header=T)
quad = read.table("out_quad", header=T)

mpfr$backend=factor("mpfr")
quad$backend=factor("quad")

data = rbind(mpfr, quad) 
print(summary(data))

print(sd(data[data$backend=="mpfr",]$c))
print(sd(data[data$backend=="quad",]$c))

print(summary(data))

ggplot(data, aes(x=backend, y=c)) + geom_violin() + geom_jitter(alpha=0.5) 




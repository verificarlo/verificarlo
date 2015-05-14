#!/usr/bin/env Rscript

require(ggplot2)

data1 = read.table("output1", header=T)
m = mean(data1$y)
data1$y = data1$y - m
data2 = read.table("output2", header=T)
data2$y = data2$y - m
data1$method="-O0"
data2$method="-O3 -ffast-math"

data = rbind(data1, data2)
print(summary(data))

ggplot(data, aes(x=y)) + geom_histogram() + facet_wrap(~method) + theme_bw()

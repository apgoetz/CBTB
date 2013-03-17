#!/usr/bin/Rscript

pdf("random.pdf")
rdata <- read.table("datasets/random_data", sep=",", header=F)
plot(rdata, main="", xlab="BTB Size", ylab="Missed Targets per 100 branches")
dev.off()

pdf("ecdf-serv-1-stack.pdf")
sdata <- read.table('datasets/serv-1-stack-depth.gz')
plot(ecdf(sdata[[1]]), main="", xlab="Stack Depth",  verticals=T, pch=46)
dev.off()

pdf("ecdf-fp-1-stack.pdf")
fpdata <- read.table('datasets/fp-1-stack-depth.gz')
plot(ecdf(fpdata[[1]]), main="", xlab="Stack Depth", verticals=T, pch=46)
dev.off()

pdf("maxdepth.pdf")
maxdepth <- read.table("datasets/stackdepth", header=T, sep="\t")
barplot(as.matrix(maxdepth), beside=T)
dev.off()


pdf("full-route-number.pdf")

# Enlarge the lower margin for longer xtic labels.
mymai <- par("mai")
mymai[1] <- mymai[1] * 1.8
par(mai = mymai)

# Read data.
# L <- read.table ("full-routes.header", sep = ",")
Z <- read.table ("full-route-number.dat", sep=",")
colnames(Z) <- c("ts", 0:(ncol(Z)-2))
Z
f <- (Z[1,] > 0)
f
Z1 <- Z[,f]
Z1

# Read labels as X.
L <- as.POSIXct(Z[,1], origin = "1970/01/01 00:00:00 UTC")
X <- data.frame(ts=Z[,1], label=as.character(L))
X
nrow(X)
n <- nrow(X) / 20
n
a <- c (T, rep(F, length = n - 1))
a
X1 <- rep(a, nrow(X) / n)
X1
X2 <- X[X1,]
X2
nrow(X2)

# xrange
xrange <- range(Z[,1])

# yrange
Z2 <- Z[,-1]
Y1 <- Z2[Z2 > 0]
Y1
Y2 <- sort(Y1)
Y2
outlier=0.2 * length(Y2)
outlier
start=outlier
start
end=length(Y2)-outlier
end
Y3 <- Y2[start:end]
Y3
yrange <- range(Y3)
yrange

# ymargin
ymargin <- (max(yrange) - min(yrange)) * 1.0
yrange <- yrange + c(-ymargin, +ymargin)
yrange
# yrange <- c(460000, 500000)

# plot.
#plot(range(Z[,1]), range(0, max(Z[,3:9])), type="n",
plot(xrange, yrange, type="n",
     xlab = "", ylab = "Number of Routes", xaxt = 'n')

cols <- rainbow(ncol(Z1)-1)
ltys <- c(1:ncol(Z1)-1)
pchs <- c(1:ncol(Z1)-1)

for (i in 1:(ncol(Z1)-1)) {
  lines(Z1[,1], Z1[,i+1], type="b", lty=ltys[i], col=cols[i], pch=pchs[i], lwd=1)
}

par(xpd = TRUE);
plotregion <- par("usr");
#colnames(plotregion) <- c("minx", "maxx", "miny", maxy");

thistitle <- "Number of Full Routes"
title(thistitle)

# legend(min(xrange), max(yrange),
#        legend = c(as.matrix(L[,3:9])), col = cols[3:9], lty = ltys[3:9],
#        pch = pchs[3:9], ncol = 2, cex=.8)

legend(min(xrange), max(yrange),
       legend = colnames(Z1[,-1]), col = cols, lty = ltys,
       pch = pchs, ncol = 8, cex=.8)

axis(1, at = X2[,1], labels = FALSE)
text(X2$ts, y = plotregion[3] - ((plotregion[4] - plotregion[3]) * 0.2),
     labels = c(as.matrix(X2$label)),
     srt = 90, cex = .8)

# dev.print(pdf, file = "full-routes.pdf")

dev.off()


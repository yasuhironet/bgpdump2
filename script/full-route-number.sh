
bgpdump2=../../src/bgpdump2
datfile=full-route-number.dat
rfile=full-route-number.R

date "+%Y-%m-%d-%H:%M:%S (%s)"
$bgpdump2 --count $* | tee $datfile
date "+%Y-%m-%d-%H:%M:%S (%s)"
R -f $rfile
echo 'done.'
date "+%Y-%m-%d-%H:%M:%S (%s)"


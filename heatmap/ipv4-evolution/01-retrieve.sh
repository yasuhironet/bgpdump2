
urlprefix="http://archive.routeviews.org/bgpdata/"

years=`seq 2009 2021`
months=`seq -w 1 12`
#days=`seq -w 1 31`

for y in $years; do
    for m in $months; do
        if [ -f $y/$m/filelist.txt ]; then
            for f in `cat $y/$m/filelist.txt`; do
                echo wget -P $y/$m -nc $urlprefix/$y.$m/RIBS/$f
                wget -P $y/$m -nc $urlprefix/$y.$m/RIBS/$f
            done
        fi
    done
done


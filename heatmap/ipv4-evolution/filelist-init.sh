
years=`seq 2009 2020`
months=`seq -w 1 12`
days=`seq -w 1 31`

for y in $years; do
    for m in $months; do
        mkdir -p $y/$m
        echo > $y/$m/filelist-init.txt
        for d in $days; do
            echo "rib.$y$m$d.0000.bz2" >> $y/$m/filelist-init.txt
        done
    done
done


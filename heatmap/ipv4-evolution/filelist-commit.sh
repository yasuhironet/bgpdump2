
years=`seq 2009 2020`
months=`seq -w 1 12`
days=`seq -w 1 31`

for y in $years; do
    for m in $months; do
        [ -f $y/$m/filelist.txt ] && git add $y/$m/filelist.txt
    done
done


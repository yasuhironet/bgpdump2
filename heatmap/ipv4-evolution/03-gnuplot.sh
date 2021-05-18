
bgpdump2=$(cd ../../src/ && pwd)/bgpdump2

dir=obj
dir2=obj2

mkdir -p $dir $dir2
cd $dir

years=`seq 2009 2020`
months=`seq -w 1 12`
# days=`seq -w 1 31`

for y in $years; do
    for m in $months; do
        if [ -f ../$y/$m/filelist.txt ]; then
            for f in `cat ../$y/$m/filelist.txt`; do
                xgp=`ls $f-p*.gp 2> /dev/null`
                xpng=`ls $f-p*.png 2> /dev/null`
                if [ -n "$xgp" -a -z "$xpng" ]; then
                    for g in `ls $f-p*.gp`; do
                        gnuplot $g
                        break;
                    done
                fi
            done
        fi
    done
done


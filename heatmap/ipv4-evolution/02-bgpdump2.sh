
bgpdump2=$(cd ../../src/ && pwd)/bgpdump2

dir=obj
dir2=obj2

mkdir -p $dir $dir2
cd $dir

years=`seq 2009 2021`
months=`seq -w 1 12`
# days=`seq -w 1 31`

for y in $years; do
    for m in $months; do
        if [ -f ../$y/$m/filelist.txt ]; then
            for f in `cat ../$y/$m/filelist.txt`; do
                res=`ls $f-p*.gp 2> /dev/null`
                [ -z "$res" ] && $bgpdump2 -H $f ../$y/$m/$f -a 2914
            done
        fi
    done
done


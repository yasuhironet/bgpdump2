
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
                [ ! -f $f-p10.gp ] && $bgpdump2 -H $f ../$y/$m/$f -p 10
                [ ! -f $f-p10.png ] && gnuplot $f-p10.gp
            done
        fi
    done
done

cd ..

count=0
for f in `ls $dir/*.png`; do
    cp $f $dir2/$count.png
    (( count++ ))
done

ffmpeg -r 30 -i $dir2/%d.png -vcodec libx264 -pix_fmt yuv420p -r 30 ipv4-evolution.mp4


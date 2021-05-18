
bgpdump2=$(cd ../../src/ && pwd)/bgpdump2

dir=obj
dir2=obj2

mkdir -p $dir $dir2

count=0
for f in `ls $dir/*.png`; do
    cp $f $dir2/$count.png
    (( count++ ))
done


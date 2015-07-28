
arg=$1
dir=${arg%/}

bgpdump=src/bgpdump2

if [ ! -d "$dir" ]; then
    echo "Usage: sh $0 <dir>"
    exit
fi

# for filepath in $dir/*.bz2; do
#     file=${filepath##*/}
#     name=${file%.bz2}
#     echo $bgpdump $filepath -H $dir/$name -a 2914
#     $bgpdump $filepath -H $dir/$name -a 2914
# done

# for filepath in $dir/*.gp; do
#     file=${filepath##*/}
#     name=${file%.*}
#     gnuplot $filepath
# done

# for filepath in $dir/*.eps; do
#     file=${filepath##*/}
#     name=${file%.*}
#     convert -density 288 $filepath $dir/$name.png
# done

list=""
for filepath in $dir/*.png; do
    file=${filepath##*/}
    name=${file%.*}
    namewop=${file%-p*}
    echo $namewop >> tmp
done

list=`cat tmp | sort | uniq`
echo $list

mkdir -p hoge
j=0
for i in $list; do
   file=`ls $dir/$i-*.png | head -1`
   jj=`printf %02d $j`
   cp $file hoge/$jj.png
   j=$(( j + 1 ))
done



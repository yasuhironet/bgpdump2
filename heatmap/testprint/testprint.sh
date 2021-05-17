
bgpdump2=$(cd ../../src/ && pwd)/bgpdump2

dir=obj

mkdir -p $dir
cd $dir

$bgpdump2 -H testprint

for i in `seq 0 255`; do
  echo gnuplot testprint-p$i.gp
  gnuplot testprint-p$i.gp
done

cd ..

ffmpeg -r 30 -i $dir/testprint-p%d.png -vcodec libx264 -pix_fmt yuv420p -r 30 testprint.mp4


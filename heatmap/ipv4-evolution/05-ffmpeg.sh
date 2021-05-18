
dir=obj
dir2=obj2

ffmpeg -r 30 -i $dir2/%d.png -vcodec libx264 -pix_fmt yuv420p -r 30 ipv4-evolution.mp4


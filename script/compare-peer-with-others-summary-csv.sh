
usage="Usage: sh $0 <rib-file> <peer-id> [<outdir>]"

echo=/bin/echo

ribfile=$1
if [ -z "$ribfile" ]; then
    $echo "specify rib-file."
    $echo $usage
    exit 1
fi

peerid=$2
if [ -z "$peerid" ]; then
    $echo "specify peer-id."
    $echo $usage
    exit 1
fi

filedir=`echo $ribfile | sed 's/\/.*//'`
filename=`echo $ribfile | sed 's/.*\///'`

outdir=$3
if [ -z "$outdir" ]; then
    outdir="data/$filename-$peerid"
fi

if [ ! -d $outdir ]; then
    $echo "no outdir found: $outdir"
    $echo $usage
    exit 1
fi

bgpdump=./src/bgpdump2
if [ ! -x $bgpdump ]; then
    $echo "compile first !"
    exit 1
fi

$echo -n "Checking peernum ..."
peernum=`$bgpdump -P $ribfile | grep 'peer_table\[[0-9]*\]' | wc -l | sed 's/[ \t]*//'`
$echo " $peernum"

$echo "ribfile: $ribfile"
$echo "peer-id: $peerid"
$echo "peernum: $peernum"
$echo "outdir: $outdir"

which jot 2>&1 > /dev/null
if [ $? = 0 ]; then
    peerlist=`jot $peernum 0`
else
    peerlist=`seq 0 $peernum`
fi

$echo $peerlist

echo "peer1,\tpeer2,\tladd,\tradd,\tlonly,\trpart,\tronly,\tlpart"

outprefix=$outdir/route-diff-$filename-p$peerid
for i in $peerlist; do
  ladd=`cat $outprefix-p$i-diff.txt | grep '^-' | wc -l | sed 's/[ \t]//g'`
  radd=`cat $outprefix-p$i-diff.txt | grep '^+' | wc -l | sed 's/[ \t]//g'`
  lonly=`cat $outprefix-p$i-diff.txt | grep '^<' | wc -l | sed 's/[ \t]//g'`
  ronly=`cat $outprefix-p$i-diff.txt | grep '^>' | wc -l | sed 's/[ \t]//g'`
  rpart=`cat $outprefix-p$i-diff.txt | grep '^)' | wc -l | sed 's/[ \t]//g'`
  lpart=`cat $outprefix-p$i-diff.txt | grep '^(' | wc -l | sed 's/[ \t]//g'`
  echo "p$peerid,\tp$i,\t$ladd,\t$radd,\t$lonly,\t$rpart,\t$ronly,\t$lpart"
done


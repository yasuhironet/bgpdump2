
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

if [ ! -d $outdir ]; then
    mkdir -p $outdir
fi

which jot 2>&1 > /dev/null
if [ $? = 0 ]; then
    peerlist=`jot $peernum 0`
else
    peerlist=`seq 0 $peernum`
fi

$echo $peerlist


outprefix=$outdir/route-diff-$filename-p$peerid
$bgpdump $ribfile -P | tee $outprefix-peer-table.txt
$bgpdump $ribfile -k | tee $outprefix-stat.txt
for i in $peerlist; do
    $bgpdump $ribfile -U -r -p $peerid -p $i | tee $outprefix-p$i-detail.txt
    $bgpdump $ribfile -u -r -p $peerid -p $i | tee $outprefix-p$i-diff.txt
    cat $outprefix-p$i-diff.txt | grep '^[>(]' | tee $outprefix-p$i-focus.txt
done 


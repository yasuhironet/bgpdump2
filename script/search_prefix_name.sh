
while read buf; do
    line=`echo $buf | tr -d '\n'`
    addr=`echo $line | sed -e 's/^.*[<>()]\([0-9]*\.[0-9]*\.[0-9]*\.[0-9]\).*/\1/'`;
    descr=`whois -h riswhois.ripe.net $addr | grep -i 'descr' | tail -1 | sed -e 's/descr://g' -e 's/ //g'`
    echo "$line\t$addr\t$descr"
done


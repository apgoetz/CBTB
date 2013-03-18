#!/bin/sh
sum=0
for i in `ls traces/ | grep -v bz2 | cut -d. -f1 ` ; do 
    #echo $i 
    result=$(./launcher.pl predictor 1,6,8,-6,2,8,64 traces/$i)
    #echo -n "$result"
    tmpval=$( echo -n "$result" | grep tpred | perl -ne '@d = split /\s+/; print $d[8]')
    sum=$(echo "scale=5; $sum + $tmpval" | bc)
#    echo $sum
done

echo $(echo "scale=5; $sum / 20" | bc)

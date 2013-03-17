#!/bin/sh
sum=0
for i in `ls traces/ | grep -v bz2 | cut -d. -f1 ` ; do 
    echo $i 
    tmpval=$(./launcher.pl predictor 1,6,8,6,2,8,21 traces/$i | grep tpred | perl -ne '@d = split / +/; print $d[8]')
    sum=$(echo "scale=5; $sum + $tmpval" | bc)
    echo $sum
done

print $(echo "scale=5; $sum / 20" | bc)

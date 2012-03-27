for i in 1 2 3 4
do
for j in 1 2 3 4
do
echo $i $j
./Proj3 trace.txt $i $j 3
echo ""
done
done

echo "LD/SD test"
for i in 1 2 3 4
do
echo $i
./Proj3 trace.txt 2 2 $i
echo ""
done


for i in {1..33}
do 
ssh -tt -o "StrictHostKeyChecking no" node-$i /root/dc.sh &
 done

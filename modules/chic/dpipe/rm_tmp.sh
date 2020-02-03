
seq 1 33 | parallel -j 33 ssh -tt node-{} rm -f /mnt/tmp/* 


#./preprocess.sh
#./rmgaps.sh

#hdfs dfs -put /media/tmp/all/hdfs /user/root/all

PGFOLDER=$1
PGPATHLOCAL=$2
LOCALINDEXPATH=/mnt/tmp

echo "Starting SAMS.."
start=`date +%s`

#mkdir -p $LOCALINDEXPATH/sams/
#seq 1 22 | xargs -I{} -n 1 -P 22 scp -o "StrictHostKeyChecking no" node-{}:$LOCALINDEXPATH/mapped*.sam $LOCALINDEXPATH/sams/

#./sams.sh $PGPATHLOCAL $LOCALINDEXPATH/sams/ > sams.log

#./sam2pos.sh $PGPATHLOCAL $LOCALINDEXPATH/sams/ > sam2pos.log

end=`date +%s`
runtime=$((end-start))
echo "SAMS: ${runtime}" >> runtime.log
#hdfs dfs -mkdir -p pos

#hdfs dfs -put $LOCALINDEXPATH/sams/*.pos pos/

#TODO do this per chr
echo "Starting adhoc construction.."
start=`date +%s`
seq 1 22 | xargs -I{} -n 1 -P 2 ./adhoc.sh {} $PGFOLDER $PGPATHLOCAL adhoc/chr{}
#for i in {21..22}
#do
# ./adhoc.sh $i $PGFOLDER $PGPATHLOCAL adhoc/chr$i
#done

end=`date +%s`
#hdfs dfs -put ${SAM_FOLDER}/*.pos pos
runtime=$((end-start))
echo "adhoc: ${runtime}" >> runtime.log

#!/usr/bin/env bash

CUR=pwd
###### USE ONLY FOR TESTING PURPOSES. USE AT YOUR OWN RISK. ######

#Assume we have 26 nodes having hostnames node-1...node-26 and we are logged in as root user
#Hadoop is installed in folder /opt/hadoop and Spark in /opt/spark on every node

#####Get Hadoop and Spark
wget https://archive.apache.org/dist/hadoop/core/hadoop-2.7.3/hadoop-2.7.3.tar.gz -O /opt/hadoop-2.7.3.tar.gz
tar -xf /opt/hadoop-2.7.3.tar.gz
mv /opt/hadoop-2.7.3 /opt/hadoop

wget http://archive.apache.org/dist/spark/spark-2.3.2/spark-2.3.2-bin-hadoop2.7.tgz -O /opt/spark-2.3.2-bin-hadoop2.7.tgz
tar -xf /opt/spark-2.3.2-bin-hadoop2.7.tgz
mv /opt/spark-2.3.2-bin-hadoop2.7 /opt/spark

wget https://github.com/BenLangmead/bowtie2/releases/download/v2.4.2/bowtie2-2.4.2-linux-x86_64.zip -O /opt/bowtie2-2.4.2-linux-x86_64.zip
unzip /opt/bowtie2-2.4.2-linux-x86_64.zip
mv /opt/bowtie2-2.4.2-linux-x86_64 /opt/bowtie

wget https://ftp.ncbi.nlm.nih.gov/blast/executables/blast+/2.8.0alpha/ncbi-blast-2.8.0-alpha+-x64-linux.tar.gz -O /opt/ncbi-blast-2.8.0-alpha+-x64-linux.tar.gz
tar -xf /opt/ncbi-blast-2.8.0-alpha+-x64-linux.tar.gz
mv /opt/ncbi-blast-2.8.0-alpha+-x64-linux /opt/blast

wget https://github.com/samtools/samtools/releases/download/1.11/samtools-1.11.tar.bz2 -O /opt/samtools-1.11.tar.bz2
tar -xf /opt/samtools-1.11.tar.bz2
mv /opt/samtools-1.11.tar.bz2  /opt/samtools
cd /opt/samtools
make /opt/samtools
cd $CUR



#Modify hosts, HDFS dirs, memory options in /opt/hadoop/etc/hadoop/core-site.xml, /opt/hadoop/etc/hadoop/hdfs-site.xml, /opt/hadoop/etc/hadoop/yarn-site.xml and /opt/spark/conf/spark-defaults.conf
#add all nodes to /etc/hosts and /opt/hadoop/etc/hadoop/workers file

## Minimal configurations

##### /opt/hadoop/etc/hadoop/core-site.xml
#<configuration>
#	<property>
#            <name>fs.default.name</name>
#            <value>hdfs://node-1:8020</value>
#        </property>
#</configuration>

##### /opt/hadoop/etc/hadoop/hdfs-site.xml
#<configuration>
#    <property>
#            <name>dfs.namenode.name.dir</name>
#            <value>/hadoop/dfs/nn</value>
#    </property>
#
#    <property>
#            <name>dfs.datanode.data.dir</name>
#            <value>/hadoop/dfs/dn</value>
#    </property>
#
#    <property>
#            <name>dfs.replication</name>
#            <value>3</value>
#    </property>
#</configuration>

##### /opt/hadoop/etc/hadoop/yarn-site.xml
#
#<configuration>
#
#<!-- Site specific YARN configuration properties -->
#    <property>
#            <name>yarn.acl.enable</name>
#            <value>0</value>
#    </property>
#
#    <property>
#            <name>yarn.resourcemanager.hostname</name>
#            <value>node-1</value>
#    </property>
#
#    <property>
#            <name>yarn.nodemanager.aux-services</name>
#            <value>mapreduce_shuffle</value>
#    </property>
#    <property>
#            <name>yarn.nodemanager.resource.memory-mb</name>
#            <value>5000</value>
#    </property>
#
#    <property>
#            <name>yarn.scheduler.maximum-allocation-mb</name>
#            <value>30000</value>
#    </property>
#
#    <property>
#            <name>yarn.scheduler.minimum-allocation-mb</name>
#            <value>1024</value>
#    </property>
#
#    <property>
#            <name>yarn.nodemanager.vmem-check-enabled</name>
#            <value>false</value>
#    </property>
#</configuration>

#### /opt/hadoop/etc/hadoop/workers

#node-2
#node-3
#node-4
#node-5
#node-6
#node-7
#node-8
#node-9
#node-10
#node-11
#node-12
#node-13
#node-14
#node-15
#node-16
#node-17
#node-18
#node-19
#node-20
#node-21
#node-22
#node-23
#node-24
#node-25
#node-26

#### /opt/spark/conf/spark-defaults.conf
#spark.eventLog.enabled           true
#spark.eventLog.dir               hdfs://localhost:8020/tmp/applicationHistory
#spark.history.fs.logDirectory	  /tmp/applicationHistory

## Environment variables, add to .bashrc
export HDFS_NAMENODE_USER="root"
export HDFS_DATANODE_USER="root"
export HDFS_SECONDARYNAMENODE_USER="root"
export YARN_RESOURCEMANAGER_USER="root"
export YARN_NODEMANAGER_USER="root"

export YARN_CONF_DIR=/opt/hadoop/etc/hadoop/
export PATH=/opt/hadoop/bin:/opt/hadoop/sbin:/opt/spark/bin:$PATH
#export JAVA_HOME=/usr/lib/jvm/jre-1.8.0-openjdk #Modify and uncomment if java is not found

#Generate ssh key
ssh-keygen
#copy ssh key to all nodes
for i in {1..26}; do ssh-copy-id -i $HOME/.ssh/id_rsa.pub root@node-$i; done


distribute(){


    #####Install java-1.8.0-openjdk
    ssh -tt node-$i yum install -y java-1.8.0-openjdk
    #Copy Hadoop and Spark
    ssh -tt node-$i mkdir /hadoop
    scp -r /opt/hadoop node-$i:/
    scp -r /opt/spark node-$i:/
    #distribute hosts file
    scp /etc/hosts node-$i:/etc/hosts

    #Copy only configurations if changed
    #scp /opt/hadoop/etc/hadoop/* node-$i:/opt/hadoop/etc/hadoop/;

    #If more disk space is needed, add volumes to all nodes and mount to /hadoop (default hdfs dir will be /hadoop/dfs assigned in /opt/hadoop/etc/hadoop/hdfs-site.xml)
    #ssh -tt node-$i mkfs.ext4 /dev/vdb
    #ssh -tt node-$i mount /dev/vdb /hadoop

    #Remove all HDFS data if needed
    #ssh -tt node-$i rm -r -f /hadoop/*
}
#change iterator to match hostnames
for i in {1..26}; do distribute "$i" & done

#start services
hdfs namenode -format
/opt/hadoop/sbin/start-all.sh
hdfs dfs -mkdir /tmp
hdfs dfs -mkdir /user
hdfs dfs -mkdir /user/root

mkdir -p /opt/chic/
cp -r ../index/ /opt/chic/
mkdir -p /mnt/tmp

#Setup worker nodes
for i in {2..26}; do
    ssh -tt -o "StrictHostKeyChecking no" node-$i mkdir /opt/chic/
    scp -o "StrictHostKeyChecking no" -r ../index/ node-$i:/opt/chic/

    ssh -tt -o "StrictHostKeyChecking no" node-$i mkdir -p /opt/chic/ext/
    scp -o "StrictHostKeyChecking no" -r  ../modules/chic/ext/BOWTIE2 node-$i:/opt/chic/ext/
    scp -o "StrictHostKeyChecking no"  -r /opt/bowtie2/ node-$i:/opt/
    scp -o "StrictHostKeyChecking no"  -r /opt/blast/ node-$i:/opt/
    scp -o "StrictHostKeyChecking no"  -r /opt/samtools/ node-$i:/opt/
    ssh -tt -o "StrictHostKeyChecking no" node-$i mkdir /mnt/tmp

    #scp -o "StrictHostKeyChecking no" /opt/hadoop/etc/hadoop/* node-$i:/opt/hadoop/etc/hadoop/ &
    #scp /etc/hosts node-$i:/etc/hosts
done
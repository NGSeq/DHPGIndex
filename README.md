DHPGIndex
=================

General
-------

This tool is for compressing and indexing pan-genomes and genome sequence collections for scalable sequence and read alignment purposes. The pipeline can be deployed in cloud computing environment or in dedicated computing cluster. The tool extends the CHIC aligner https://gitlab.com/dvalenzu/CHIC with distributed and scalable features. DHPGIndex have been tested with bacterial and human genomes, but could be harnessed for other species with little modifications.


Requirements
------------

* python2.7 or newer
* spark 2.0 or newer
* yarn
* HDFS


Installation
-------

Compile and Install needed packages by running:
```bash
./compile.sh
```

### Cluster setup
Automated installation with Hadoop distributions such as Cloudera CDH or by using some cloud service provider.

or

Manually with minimal configuration by following the instructions in sbin/spark-hadoop-init.sh and running the script.
Modify ssh credentials, node names and numbering in the script corresponding to your system.


Experimenting with microbial pan-genome (BLAST index is used by default)
---
### Running simple test with small E.coli data (default configuration uses only a single node)
---
```bash
cd sbin
./test.sh
```
---
### Preparing real data
---
Download microbial genomes in FASTA format e.g. from NCBI assembly database to local filesystem, e.g, under /data/pangenome/ folder.

---
### Modify variables
---

To run the whole pipeline bash script `pipeline_microbes.sh` is used.
Modify the number of cluster nodes and static paths to fit your configuration in the `pipeline_microbes.sh` file.

LOCALINDEXPATH=/mnt/tmp #Intermediate files and the final index are stored here

HDFSURI=hdfs://node-1:8020 #HDFS namenode address

NODE=node- #Basename for nodes in the cluster. The nodes must be numbered starting from 1.

---
### Launching
---

```bash
cd sbin
./pipeline_microbes.sh /data/pangenome/ /data/querysequences.fna
```
Here the first parameter is a local filesystem folder where the pangenome sequence files are in FASTA format.
The pangenome data is automatically uploaded to the HDFS under the 'pg' folder.
The second parameter is the file including query sequences that are aligned eventually against created pan-genomic index.

Experimenting with human pan-genome (Bowtie index is used by default)
---
### Preparing data
---
Download standard human genome version GRCh37 chromosomes 1-22 from https://ftp.ncbi.nlm.nih.gov/genomes/archive/old_genbank/Eukaryotes/vertebrates_mammals/Homo_sapiens/GRCh37/Primary_Assembly/assembled_chromosomes/FASTA/ and unzip to /data/grch37 folder.
Download phased SNP genotype data (includes both haplotypes) from ftp://ftp.1000genomes.ebi.ac.uk/vol1/ftp/release/20130502/ for chromosomes 1-22 to /data/vcfs. Unzip and rename to chr1..22.vcf. 
The whole data set includes 2506 samples generating 13 TB pangenome! You can split the VCF files to smaller test sets eg. 50 samples with command 'cut -f1-59 chr1.vcf'.

or

Use already assembled sequences and skip the vcf2multialign stage (comment out lines in pipeline_hg.sh). Copy the assembled sequences in /data/pangenome/.

---
### Modify variables
---

To run the whole pipeline bash scirpt `pipeline_hg.sh` is used.
The initial configuration assumes at least 22 worker nodes for running chromosomes 1-22 in some stages in parallel (if less used only the numbered chromosomes are assembled).
Modify static paths to fit your configuration in the `pipeline_hg.sh`

LOCALINDEXPATH=/mnt/tmp #Intermediate files and the final index are stored here

STDREFPATH=/data/grch37 #Standard reference genome files in FASTA format. Files must be divided by chromosomes and named chrN.fa N=1..22

VCFPATH=/data/vcfs #VCF files aligned to standard reference with vcf2multialign to generate pangenome. VCF files must be divided by chromosomes and named chrN.vcf N=1..22

HDFSURI=hdfs://node-1:8020 #HDFS namenode address

NODE=node- #Basename for nodes in the cluster. The nodes must be numbered starting from 1.

---
### Launching
---

```bash
cd sbin
./pipeline_hg.sh /data/pangenome/ /data/ngsreads/reads_1.fastq /data/ngsreads/reads_2.fastq
```
Here the first parameter is a local filesystem folder where the pangenome sequence files are generated to.
The pangenome data is automatically uploaded to the HDFS under the same folder.
The next two parameters are are local paired-end read files that are are also uploaded to HDFS and aligned eventually against the pan genome index.


Note
------------------

* At least 2x size of pan-genome in master node is required for local storage and worker nodes should have storage at least 2x of chromosomal pan-genome data size.
* Everything is tested using CentOS 7 with openjdk 1.8




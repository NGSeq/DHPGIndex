Pangendex
=================

General
-------

This tool is for compressing and indexing pan-genomes and genome sequence collections for scalable sequence and read alignment purposes. The pipeline can be deployed in cloud computing clusters and used with any species. The tool extends the CHIC aligner https://gitlab.com/dvalenzu/CHIC with distributed and scalable features.


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
Modify the ssh credentials, node names and numbering at the end of the script corresponding to your system.

### Cluster setup
Automated installation with Hadoop distributions such as Hortonworks HDP or Cloudera CDH.

or

Manually with minimal configuration by following the instructions in sbin/spark-hadoop-init.sh and running the script.


Exmperimenting
---
### Preparing data
---
Download standard human genome version GRCh37 chromosomes 1-22 from https://ftp.ncbi.nlm.nih.gov/genomes/archive/old_genbank/Eukaryotes/vertebrates_mammals/Homo_sapiens/GRCh37/Primary_Assembly/assembled_chromosomes/FASTA/ and unzip to /data/grch37 folder.
Download phased SNP genotype data (includes both haplotypes) from ftp://ftp.1000genomes.ebi.ac.uk/vol1/ftp/release/20130502/ for chromosomes 1-22 to /data/vcfs. Unzip and rename to chr1..22.vcf. 
The whole data set includes 2506 samples generating 13 TB pangenome! You can split the VCF files to smaller test sets eg. 50 samples with command 'cut -f1-59 chr1.vcf'.

or

Use allready assembled sequences and skip the vcf2multialign stage (uncomment lines in launch.sh). Copy the assembled sequences in /data/pangenome/.

---
### Modify variables
---

To run the whole pipeline bash scirpt `launch.sh` is used.
The initial configuration assumes at least 22 worker nodes for running chromosomes 1-22 in some stages in parallel (if less used only the numbered chromosomes are assembled).
Modify static paths to fit your configuration in the launch.sh

LOCALINDEXPATH=/mnt/tmp # Should be same as was created with the compile.sh

STDREFPATH=/data/grch37 #Standard reference genome files in FASTA format. Files must be divided by chromosomes and named chrN.fa N=1..22

VCFPATH=/data/vcfs #VCF files aligned to standard reference with vcf2multialign to generate pangenome. VCF files must be divided by chromosomes and named chrN.vcf N=1..22

HDFSURI=hdfs://namenode:8020

NODE=node- #Basename for nodes in the cluster. The nodes must be numbered starting from 1.

---
### Launching
---

```bash
./pipeline_hg.sh /data/pangenome/ /data/ngsreads/reads.fq.1.fastq /data/ngsreads/reads.fq.2.fastq
```
Here the first parameter is a local filesystem folder where the pangenome sequence files are generated to.
The pangenome data is automatically uploaded to the HDFS under the same folder.
The next two parameters are are local paired-end read files that are are also uploaded to HDFS and aligned eventually against the pan genome index.

Note
------------------

* At least 2x size of pan-genome in master node is required for local storage and worker nodes should have storage at least 2x of chromosomal pan-genome data size.
* Everything is tested using CentOS 7 with openjdk 1.8
* Each FASTA file should contain a single line.




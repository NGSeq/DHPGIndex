package org.ngseq.panquery

import org.apache.hadoop.conf.Configuration
import org.apache.hadoop.fs.{FileStatus, FileSystem, Path}
import org.apache.spark.sql.SparkSession

import scala.collection.mutable.ArrayBuffer
import scala.sys.process._

object VCFMultialign2 {


  // [reference FASTA file] [VCF file] [vcfalign path] [path for splitted vcf files] []
  def main(args: Array[String]) {

    val reference = args(0)
    val vcfPath = args(1)
    val vcfAlignPath = args(2)
    val hdfsOut = args(3)
    val output = args(4)

    val spark = SparkSession.builder.appName("VCFMultialign").getOrCreate()

    val fs = FileSystem.get(new Configuration())
    //list files in the hdfs directroty
    val st: Array[FileStatus] = fs.listStatus(new Path( vcfPath ))
    //and add to array
    var splitFileList = ArrayBuffer[String]()
    st.map(file_st => {
      if (!file_st.equals("_SUCCESS")){
        splitFileList.append(file_st.getPath.getName)
      }
    })

    val vcfFilesRDD = spark.sparkContext.parallelize(splitFileList, splitFileList.length)


    /*val header = vcfData.filter(_.startsWith("#"))

    val data = vcfData.filter(!_.startsWith("#")).map(_.split("\t"))

    val distinctIds = data.flatMap(x => x(2).split(";")).distinct 
    //find distinct ids
    val ids = distinctIds.collect().par

    //save smaller files
    for(i <- ids) {
      //append header
      header.union(data.filter(x => x(2).split(";").contains(i)).map(_.mkString("\t"))).coalesce(1).write.text(hdfsOut+i+".vcf")
    }

    val commands = distinctIds.map{x =>
      vcfAlignPath+" --reference="+reference+" --variants=" #< "(hdfs dfs -text " + hdfsOut + x + ") --output-reference="+output !
    }
    //trigger calculation
    commands.count()*/

    spark.stop()
  }
}
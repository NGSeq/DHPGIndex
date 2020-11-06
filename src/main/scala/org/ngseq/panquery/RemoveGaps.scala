package org.ngseq.panquery

import java.io.{File, PrintWriter}
import java.text.DecimalFormat

import org.apache.spark.sql.SparkSession

object RemoveGaps {

  def main(args: Array[String]) {

    val spark = SparkSession.builder.appName("RMGAPS").getOrCreate()
    import spark.implicits._

    val dataPath = args(0)
    val filerange = args(1)
    val output = args(2)

    val sp = filerange.split("-")
    val start = sp(0).toInt
    val end = sp(1).toInt
    val len = end-start
    val nf = new DecimalFormat("00000")

    val ls = Seq.tabulate(len)(n => dataPath+"/"+nf.format(start+n)+".*")

    val local = spark.read.text("{"+ls.mkString(",")+"}")
      .select(org.apache.spark.sql.functions.input_file_name, $"value")
      .as[(String, String)]
      .rdd.map{v=>
      //val groups = v.grouped(x._1._2.length()/numSplits).toArray
      //groups.zipWithIndex.map(y => (fileName,y._2,x._2,y._1))
      val gapless = v._2.replaceAll("-", "")

      val fname = v._1.toString.split("/")
      val header = ">"+fname(fname.length-1)

      (header,gapless)
    }.sortBy(_._1).toLocalIterator

    //val bos = new BufferedOutputStream(new FileOutputStream(output,true))
    val pw = new PrintWriter(new File(output))
    //pw.write(">"+ref._1.split("/").last+"\n")
    //nref.foreach(x => pw.write(x._2))
    //pw.close()
    val tmp = local.next
    pw.write(tmp._1)
    pw.write(System.lineSeparator())
    pw.write(tmp._2)
    while(local.hasNext) {
      val tmp = local.next
      //println(tmp._1+","+tmp._2)
      pw.write(System.lineSeparator())
      pw.write(tmp._1)
      pw.write(System.lineSeparator())
      pw.write(tmp._2)
    }
    pw.close()
  }
}

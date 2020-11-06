package org.ngseq.panquery

import java.io.{BufferedOutputStream, FileOutputStream}
import java.text.DecimalFormat

import org.apache.spark.sql.SparkSession

object LoadLZ {

  def main(args: Array[String]) {

    val spark = SparkSession.builder.appName("LoadLZ").getOrCreate()

    val dataPath = args(0)
    val filerange = args(1)
    val output = args(2)

    val sp = filerange.split("-")
    val start = sp(0).toInt
    val end = sp(1).toInt
    val len = end-start
    val nf = new DecimalFormat("0000")

    val ls = Seq.tabulate(len)(n => dataPath+"/"+"*_"+nf.format(start+n)+"_*")

    val local = spark.sparkContext.binaryFiles("{"+ls.mkString(",")+"}").sortBy(_._1).toLocalIterator

    val bos = new BufferedOutputStream(new FileOutputStream(output,true))
    //val pw = new PrintWriter(new File(output))
    //pw.write(">"+ref._1.split("/").last+"\n")
    //nref.foreach(x => pw.write(x._2))
    //pw.close()

    while(local.hasNext) {
      val tmp = local.next
      //println(tmp._1+","+tmp._2)
      //bos.write(System.lineSeparator())
      bos.write(tmp._2.toArray())
      //pw.write(System.lineSeparator())
      //pw.write(tmp._2)
    }
    bos.close()
  }
}

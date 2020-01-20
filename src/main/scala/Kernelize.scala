package org.ngseq.panquery

import org.apache.spark.sql.SparkSession

object Kernelize {

  def main(args: Array[String]) {

    val spark = SparkSession.builder.appName("DRLZ").getOrCreate()
    import spark.implicits._

    val localradix = args(0)
    val localref = args(1)

    val dataPath = args(2)
    val hdfsurl = args(3)
    val hdfsout = args(4)

    val radixSA = "./radixSA"

    val localOut = "radixout.txt"
    val refParse = "./rlz_for_hybrid"
    val output = "merged.lz"


    println("Load suffix")
    val suffix = scala.io.Source.fromFile(localradix).getLines.toArray.map(_.toInt)

    println("broadcasting")
    val SA = spark.sparkContext.broadcast(suffix)

    // broadcast plain ref (needed for pattern matching)
    //val reference = spark.sparkContext.broadcast(">"+ref._1.split("/").last+"\n"+ref._2)
    val ref = scala.io.Source.fromFile(localref).getLines.mkString("")

    val reference = spark.sparkContext.broadcast(ref)


    val splitted = spark.read.text(dataPath)
      .select(org.apache.spark.sql.functions.input_file_name, $"value")
      .as[(String, String)]
      .rdd.map{v=>
      //val groups = v.grouped(x._1._2.length()/numSplits).toArray
      //groups.zipWithIndex.map(y => (fileName,y._2,x._2,y._1))
      (v._1,v._2.length,v._2)
    }


    spark.stop()

  }
}

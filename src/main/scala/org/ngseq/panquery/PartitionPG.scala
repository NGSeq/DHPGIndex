package org.ngseq.panquery

import java.io.IOException
import java.net.URI

import org.apache.hadoop.conf.Configuration
import org.apache.hadoop.fs.{FSDataOutputStream, FileSystem, Path}
import org.apache.spark.sql.SparkSession

import scala.collection.mutable.ArrayBuffer

object PartitionPG {

  def main(args: Array[String]) {

    val spark = SparkSession.builder.appName("PArtition").getOrCreate()

    val dataPath = args(0)
    val hdfsout = args(1)
    val chunksize = args(2).toInt
    val hdfsurl  = args(3)

    val chunks = spark.sparkContext.wholeTextFiles(dataPath)
      .flatMap{v=>

      val groups = v._2.grouped(chunksize).toArray
        println(v._1+"GL "+groups.length)

        val b = ArrayBuffer[(String,String,Int)]()

        var i = 0
        groups.foreach{g=>
          b+=(Tuple3(v._1,g.replaceAll("-", ""),i))
          i+=1
        }
      b

    }
    chunks.foreachPartition{chuks=>
      var fos: FSDataOutputStream = null
      val fis = FileSystem.get(new URI(hdfsurl),new Configuration())
      while(chuks.hasNext){
        try {
          val chunk = chuks.next()
          //val nf = new DecimalFormat("#0000000")
          val fname = chunk._1.split("/")
          println(hdfsout+"/" + fname(fname.length-1)+"_"+chunk._3)
          println("LEN "+chunk._2.length)

          fos = fis.create(new Path(hdfsout+"/" + fname(fname.length-1)+"_"+chunk._3))
          fos.writeBytes(chunk._2)
          fos.close()

        } catch {
          case e: IOException =>
          //e.printStackTrace()
        }
      }
      fis.close()

    }
  }
}

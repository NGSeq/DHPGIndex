package org.ngseq.panquery

import java.io._
import java.net.URI
import java.nio.ByteBuffer
import java.text.DecimalFormat

import org.apache.hadoop.conf.Configuration
import org.apache.hadoop.fs.{FSDataOutputStream, FileSystem, Path}
import org.apache.hadoop.io.{LongWritable, Text}
import org.apache.hadoop.mapreduce.lib.input.TextInputFormat
import org.apache.spark.sql.SparkSession

import scala.collection.mutable.ArrayBuffer
import scala.util.control.Breaks._

object DistributedRLZFasta {

  def main(args: Array[String]) {

    val spark = SparkSession.builder.appName("DRLZ").getOrCreate()

    val nf2 = new DecimalFormat("#00")
    val chr = nf2.format(args(0).toInt)
    val dataPath = args(1)
    val hdfsurl = args(2)
    val hdfsout = args(3)
    val refsize = args(4).toInt
    val sasplitsize  = args(5).toInt
    //val refsplitsize  = args(6).toInt
    val chunks = args(6).toInt
    val radixSA = "/opt/chic/radixSA"

    val localOut = "/mnt/tmp/radixout."+chr

    println("Load and preprocess pan-genome")
    spark.sparkContext.hadoopConfiguration.set("textinputformat.record.delimiter",">")
    val conf = new Configuration(spark.sparkContext.hadoopConfiguration)
    conf.set("textinputformat.record.delimiter", ">")

    val data = spark.sparkContext.newAPIHadoopFile(dataPath, classOf[TextInputFormat], classOf[LongWritable], classOf[Text], conf).filter(x=>x._2.getLength!=0)

    val splitted = data.flatMap{r=>
       //val gapless = v._2.replaceAll("-", "")

       val v = r._2.toString.split(System.lineSeparator())
       val groups = v(1).grouped(v(1).length/chunks).toArray
       groups.zipWithIndex.map(y => (v(0),y._1.length,y._1,y._2))

     }.groupBy(g => g._4)

    println("Divided pan-genome to "+splitted.getNumPartitions+" groups")
    println("Started distributed RLZ")
    splitted.foreach{group=>

      val refs = group._2.take(refsize).map(s=>s._3).mkString
      val reflength = refs.length

      val sar = new SAR()
      val SA = sar.suffixArray(refs)


    def getsuf(lb: Int) : Int = {

      SA(lb)

    }

    def getref(lb: Int) : Char = {

      if(lb>=refs.length)
        return "N"(0)
      refs(lb)

    }


    //val d = "cabbaabba"
    //val x = "ncabbaaabbaaa"

    // binary search that can find the upper and lower bounds
    // e.g for string 1111222555555666677 would return
    // (7,12) if we were trying to find 5
    // finds the interval for longest match
    // if i = 0 match length is 1. By increasing i
    // the ith positions are compared
    // essentially to find the longest match this function needs to called in loop
    // until the interval does not decrease
    def binarySearch(lb: Int, rb: Int, cur: Char, i: Int): Option[(Int, Int)] = {

      var low = lb
      var high = rb
      while (low < high) {
        val mid = low + ((high - low) / 2)
        // get the true position
        val midKey = getsuf(mid) + i

        // different "layers"
        val midValue = if (midKey < reflength) {
          getref(midKey)
        } else {
          '1'
        }
        //println("lb: " + low + " rb: " + high + " mid: " + mid + " key: " + midValue)

        if (cur <= midValue) {
          high = mid
        } else {
          low = mid + 1
        }
      }
      val low_res = low

      // break if key not found
      if ((getsuf(low_res) + i)>= reflength || getref(getsuf(low_res) + i) != cur) {
        return None
      }
      high = rb
      while (low < high) {
        val mid = low + ((high - low) / 2)
        val midKey = getsuf(mid) + i
        // different "layers"
        val midValue = if (midKey < reflength) {
          getref(midKey)
        } else {
          '1'
        }
        //println("lb: " + low + " rb: " + high + " mid: " + mid + " key: " + midValue)
        if (cur >= midValue) {
          low = mid + 1
        } else {
          high = mid
        }
      }
      //println("value: " + d(SA.value(low) + i) + " cur: " + cur + " lo: " + low)
      if (getsuf(low) != reflength - 1 && getsuf(low)+i< reflength && getref(getsuf(low) + i) != cur) {
        return Some((low_res, low - 1))
      }
      Some((low_res, low))
    }

    // check newline to deal with partition borders (stop phrase search if goes
    // to newline
    def factor(i: Int, x: String): (String, Long) = {

      var lb = 0
      var rb = reflength-1 // check suffix array size
      var j = i
      breakable {
        while (j < x.length()) {
          //println("j: " + j + " SA.value: " + SA.value(lb))
          //println((SA.value(lb)+j-i) + " " + d.length())
          if (lb == rb && getref(getsuf(lb) + j - i) != x(j)) {
            break
          }
          //(lb,rb) = refine(lb,rb,j-i,x(j))
          val tmp = binarySearch(lb, rb, x(j), j - i)
          //println(tmp)

          // perhaps needs more rules
          if (tmp == None) {
            break
          } else {
            //println("jassoo")
            val tmp_tuple = tmp.get
            lb = tmp_tuple._1
            rb = tmp_tuple._2
          }
          j += 1
          // border
          if (j == x.length()) {
            break
          }
        }
      }
      //println("out")
      if (j == i) {
        return (x(j).toString(), 0)
      } else {
        //println("täällä")

        return (getsuf(lb).toString(), j - i)
      }
    }

    // encode a single substring x
    // finds the longest possible match and returns
    // (pos,len) pair(s)
    def encode(x: String): ArrayBuffer[(String, Long)] = {
      var i: Int = 0
      val max = Int.MaxValue
      val output = ArrayBuffer[(String, Long)]()

      while (i < x.length()) {
        //println(i)

        val tup = factor(i, x)
        //println("<<<<<<<\n"+tup+"\n<<<<<<<")
        output += tup
        if (tup._2 == 0) {
          i += 1
        } else {
          if(i+tup._2>=max) {
            i = x.length()
          } else {
            i += tup._2.toInt
          }
        }
      }
      return output
    }
    println("started encoding")

    //var sampleid = 0
    group._2.foreach{sample =>
      //println("GROUP: "+x._2+" "+x._3.length+" "+x._4+" REFL: "+ reflength)


      var fos: FSDataOutputStream = null
      val fis = FileSystem.get(new URI(hdfsurl),new Configuration())
      val nf = new DecimalFormat("#0000")
      val fname = sample._1+"_"+nf.format(group._1)+"."+chr //.toString.split("/")

      val exists = fis.exists(new Path(hdfsout+"/" + fname))
      if(exists){
        println("File" +fname+ " exists!")
      }else{
        try {

          fos = fis.create(new Path(hdfsout+"/" + fname))

        } catch {
          case e: IOException =>
            e.printStackTrace()
        }

        val encodings = encode(sample._3)

        encodings.foreach{z =>
          var posBytes: Array[Byte] = null

          val len = z._2
          if(len != 0) {
            posBytes = ByteBuffer.allocate(8).putLong(z._1.toLong).array.reverse
          }
          else {
            posBytes = ByteBuffer.allocate(8).putLong(z._1(0).toLong).array.reverse
          }
          val lenBytes = ByteBuffer.allocate(8).putLong(len).array.reverse

          fos.write(posBytes)
          fos.write(lenBytes)
        }
        fos.close()
      }


      //sampleid+=1
    }


    }

    spark.stop()

  }
}

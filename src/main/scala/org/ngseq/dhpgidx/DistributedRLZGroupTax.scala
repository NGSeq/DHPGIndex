package org.ngseq.dhpgidx

import java.io._
import java.net.URI
import java.nio.ByteBuffer
import java.text.DecimalFormat

import org.apache.hadoop.conf.Configuration
import org.apache.hadoop.fs.{FSDataOutputStream, FileSystem, Path}
import org.apache.spark.sql.SparkSession
import org.apache.spark.storage.StorageLevel

import scala.collection.mutable.ArrayBuffer
import scala.util.control.Breaks._

object DistributedRLZGroupTax {

  def main(args: Array[String]) {

    val spark = SparkSession.builder.appName("DRLZ").getOrCreate()

    val dataPath = args(0)
    val hdfsurl = args(1)
    val lzout = args(2)
    val refquotient = args(3).toDouble
    val maxrefs = args(4).toInt
    val groupedout = args(5)

    println("Load and preprocess pan-genome")
    spark.sparkContext.hadoopConfiguration.set("textinputformat.record.delimiter",">")

    val prep = spark.sparkContext.textFile(dataPath).filter(x=>x.length!=0)

    val splitted = prep.zipWithIndex().map{rec=>

        val id = rec._2
        val seqname = rec._1.substring(1,rec._1.indexOf(System.lineSeparator))

        var seq=""
        var groupname = ""

          seq = rec._1.substring(rec._1.indexOf(System.lineSeparator)).trim

          val taxsplit = seqname.split(" ")
          var chr = ""
          if (seqname.toLowerCase.indexOf("chromosome") > -1) {
            val s1 = seqname.toLowerCase.split("chromosome")
            if (s1.length > 1)
              chr = "chr" + s1(1).split(",")(0).replaceAll("[^A-Za-z0-9]", "").trim
          }
          if (taxsplit.length > 0)
            groupname = taxsplit(0).replaceAll("[^A-Za-z0-9]", "")
          if (taxsplit.length > 1)
            groupname = taxsplit(1).replaceAll("[^A-Za-z0-9]", "")
          if (taxsplit.length > 2)
            groupname += taxsplit(2).replaceAll("[^A-Za-z0-9]", "")
          groupname += "_" + chr
          if(groupname.length>200)
            groupname = groupname.substring(0,200)

        Tuple5(id,seq.length,seq, groupname, seqname)


    }.groupBy(g=>g._4).persist(StorageLevel.MEMORY_AND_DISK_SER)

    println("Started distributed RLZ")
    splitted.foreach{group=>

      println("Started compressing tax group"+ group._1)
      val completes = group._2.filter(g=>g._5.toLowerCase.contains("complete genome"))
      var refs = ""
      if(completes.size>maxrefs)
        refs = completes.take(maxrefs).map(s=>s._3).mkString
      else
        refs = completes.map(s=>s._3).mkString

      val notcompletes = group._2.filter(g=>g._5.toLowerCase.contains("complete genome")==false)
      val seqs = notcompletes.toArray.sortBy(_._2)(Ordering[Int].reverse)
      var dictrefs = (seqs.length*refquotient).toInt
      if(dictrefs>maxrefs)
        dictrefs = maxrefs
      if(seqs.length<15)
        dictrefs = seqs.length
      refs+=seqs.take(dictrefs).map(s=>s._3).mkString

      val reflength = refs.length
      println("Creating Suffix Array from reference sequence of length" +reflength)
 
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
          if (lb == rb && getref(getsuf(lb) + j - i) != x(j)) {
            break
          }

          val tmp = binarySearch(lb, rb, x(j), j - i)

          if (tmp == None) {
            break
          } else {
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
      if (j == i) {
        return (x(j).toString(), 0)
      } else {
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

        val tup = factor(i, x)
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

      var fos: FSDataOutputStream = null
      var fos2: FSDataOutputStream = null

      val fis = FileSystem.get(new URI(hdfsurl),new Configuration())
      try {
        val nf = new DecimalFormat("#0000000000")
        val fname = nf.format(group._2.take(1).toIterator.next()._1)+"_"+group._1 //.toString.split("/")

        fos = fis.create(new Path(lzout+"/" + fname+".lz"))
        fos2 = fis.create(new Path(groupedout+"/" + fname+".fa"))

      } catch {
        case e: IOException =>
         e.printStackTrace()
      }
    group._2.foreach{sample =>
      val encodings = encode(sample._3)

      try {
        fos2.writeBytes(">" + sample._5 + System.lineSeparator() + sample._3 + System.lineSeparator())
      }catch {
        case e: NullPointerException =>
          e.printStackTrace()
      }

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

        try {
          fos.write(posBytes)
          fos.write(lenBytes)
        }catch {
          case e: NullPointerException =>
          e.printStackTrace()
        }
      }

    }
      fos.close()
      fos2.close()
    }

    spark.stop()

  }
}

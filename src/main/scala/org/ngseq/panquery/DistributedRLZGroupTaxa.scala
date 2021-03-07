package org.ngseq.panquery

import java.io._
import java.net.URI
import java.nio.ByteBuffer
import java.text.DecimalFormat

import org.apache.hadoop.conf.Configuration
import org.apache.hadoop.fs.{FSDataOutputStream, FileSystem, Path}
import org.apache.spark.TaskContext
import org.apache.spark.sql.SparkSession

import scala.collection.mutable.ArrayBuffer
import scala.util.control.Breaks._



object DistributedRLZGroupTaxa {

  def main(args: Array[String]) {

    val spark = SparkSession.builder.appName("DRLZ").getOrCreate()

    val dataPath = args(0)
    val hdfsurl = args(1)
    val lzout = args(2)
    val preprocout = args(3)
    val refquotient = args(4).toDouble
    val filterlen = args(5).toInt
    val preprocesandsave = args(6)


    //val refsplitsize  = args(6).toInt

    println("Load and preprocess pan-genome")
    spark.sparkContext.hadoopConfiguration.set("textinputformat.record.delimiter",">")
    val conf = new Configuration(spark.sparkContext.hadoopConfiguration)
    conf.set("textinputformat.record.delimiter", ">")

    val prep = spark.sparkContext.textFile(dataPath).filter(x=>x.length!=0)
    if(preprocesandsave=="1") {
      prep.map { fa =>
        val header = fa.substring(0, fa.indexOf(System.lineSeparator))
        val seq = fa.substring(fa.indexOf(System.lineSeparator)).replaceAll(System.lineSeparator(), "")
        if(seq.length<filterlen)
          (">")
        else (">" + header + System.lineSeparator() + seq)
      }.filter(x=>x.length>filterlen).saveAsTextFile(preprocout)
    }

    val splitted = prep.map{rec=>

      val seqname = rec.substring(1,rec.indexOf(System.lineSeparator))

      var seq=""
      var groupname = ""

      seq = rec.substring(rec.indexOf(System.lineSeparator)).trim
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

      //System.out.println(groupname+" "+seqname)
      Tuple4(seq.length,seq, groupname, seqname)


    }

    println("Divided pan-genome to "+splitted.getNumPartitions+" partitions")
    println("Started distributed RLZ")
    splitted.foreachPartition{part=>

      val data = ArrayBuffer.empty[Tuple4[Int, String, String, String]]
      while(part.hasNext)
        data.+=(part.next())

      var refs = ""
      var totlen = 0
      data.foreach(totlen+=_._1)
      var dictlen=totlen*refquotient
      println("DATA0"+data(0)._4)

      val gr = data.groupBy(_._3).map(d=>Tuple2((d._2.length),(d._2))).toArray.sortBy(_._1)(Ordering[Int].reverse)

      val dicts = ArrayBuffer.empty[Tuple4[Int, String, String, String]]

      var top = 0
      var len = 0

      breakable{
        gr.foreach(g=>{
            g._2.foreach(s=>{
              dicts.+=(s)
              len+=s._1
              if(len>dictlen)
                break()
            })
            top=g._1
        })
      }

      refs+=dicts.map(s=>s._2).mkString

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

      var fos: FSDataOutputStream = null
      val fis = FileSystem.get(new URI(hdfsurl),new Configuration())
      try {
        val nf = new DecimalFormat("#0000000")

        val fname = "part-"+nf.format(TaskContext.getPartitionId()) //.toString.split("/")

        fos = fis.create(new Path(lzout+"/" + fname+".lz"))

      } catch {
        case e: IOException =>
        //e.printStackTrace()
      }
    //var sampleid = 0

    data.foreach(sample=>{
      //println("GROUP: "+x._2+" "+x._3.length+" "+x._4+" REFL: "+ reflength)
      val encodings = encode(sample._2)

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

      //sampleid+=1
    })
      fos.close()
      //val delref = new File(reffile).delete()
      //      val delrad = new File(radixout).delete()
    }

    spark.stop()

  }
}

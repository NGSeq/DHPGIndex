package org.ngseq.panquery

import java.io._
import java.net.URI
import java.nio.ByteBuffer
import java.text.DecimalFormat

import org.apache.hadoop.conf.Configuration
import org.apache.hadoop.fs.{FSDataOutputStream, FileSystem, Path}
import org.apache.spark.sql.SparkSession
import org.apache.spark.storage.StorageLevel

import scala.collection.mutable.ArrayBuffer
import scala.sys.process.Process
import scala.util.control.Breaks._

object DistributedRLZPartitions {


  def main(args: Array[String]) {

    val spark = SparkSession.builder.appName("DRLZ").getOrCreate()

    val nf2 = new DecimalFormat("#00")
    val dataPath = args(0)
    val hdfsurl = args(1)
    val hdfsout = args(2)
    val refsize = args(3).toInt
    val sasplitsize  = args(4).toInt
    val maxrefs = args(5).toInt

    //val refsplitsize  = args(6).toInt
    val radixSA = "/opt/chic/radixSA"

    val localOut = "/mnt/tmp/radixout"

    println("Load and preprocess pan-genome")
    spark.sparkContext.hadoopConfiguration.set("textinputformat.record.delimiter",">")
    val conf = new Configuration(spark.sparkContext.hadoopConfiguration)
    conf.set("textinputformat.record.delimiter", ">")

    val data = spark.sparkContext.textFile(dataPath).filter(x=>x.length!=0)

    val splitted = data.zipWithIndex().map{rec=>
      val id = rec._2
      val seq = rec._1.substring(rec._1.indexOf(System.lineSeparator)).trim
      //val groups = v.grouped(x._1._2.length()/numSplits).toArray
      //groups.zipWithIndex.map(y => (fileName,y._2,x._2,y._1))
      //if(taxadepth==1)

      (id,seq.length,seq)
    }.persist(StorageLevel.MEMORY_ONLY_SER)

    println("Divided pan-genome to "+splitted.getNumPartitions+" partitions")
    println("Started distributed RLZ")
    splitted.foreachPartition{p=>
      println("Started compressing ")
      val group = p.toArray
      var dictrefs = (group.size/refsize)+1
      if(dictrefs>maxrefs)
        dictrefs = maxrefs
      val refs = group.take(dictrefs)
      val id = group(0)._1
      val reffile = "/mnt/tmp/ref"+id
      val pw = new PrintWriter(new File(reffile))
      refs.foreach(x => pw.write(x._3))
      pw.close()
      val ref = scala.io.Source.fromFile(reffile).getLines().mkString

      val reflength = ref.length
      var radixparams = ""
      if(reflength>800000000)
        radixparams = "-w "
      val radixout = localOut+"_"+id
      println("Creating Suffix Array from reference sequence of length" +reflength)
      val createSA = Process(radixSA + " "+radixparams+" "+reffile+" " + radixout).!
      /*
      println("removing radix")
      val removed = new File(localOut).delete()*/


      val sa1 = ArrayBuffer[(String)]()
      val sa2 = ArrayBuffer[(String)]()
      val sa3 = ArrayBuffer[(String)]()
      val sa4 = ArrayBuffer[(String)]()
      val sa5 = ArrayBuffer[(String)]()
      val sa6 = ArrayBuffer[(String)]()
      val sa7 = ArrayBuffer[(String)]()
      val sa8 = ArrayBuffer[(String)]()
      val sa9 = ArrayBuffer[(String)]()
      val sa10 = ArrayBuffer[(String)]()
      val sa11 = ArrayBuffer[(String)]()
      val sa12 = ArrayBuffer[(String)]()
      val sa13 = ArrayBuffer[(String)]()
      val sa14 = ArrayBuffer[(String)]()
      val sa15 = ArrayBuffer[(String)]()
      val sa16 = ArrayBuffer[(String)]()

      val is = scala.io.Source.fromFile(radixout).getLines()
      println("removing")

      val chunksize = (reflength/sasplitsize)+1
      println("Splitting Suffix Array to " +sasplitsize+" chunks of size "+chunksize)
      for(i <- 0 to sasplitsize-1){

        i match {
          case 0  => sa1.appendAll(is.slice(0,chunksize).toTraversable)
          case 1  => sa2.appendAll(is.slice(0,chunksize).toTraversable)
          case 2  => sa3.appendAll(is.slice(0,chunksize).toTraversable)
          case 3  => sa4.appendAll(is.slice(0,chunksize).toTraversable)
          case 4  => sa5.appendAll(is.slice(0,chunksize).toTraversable)
          case 5  => sa6.appendAll(is.slice(0,chunksize).toTraversable)
          case 6  => sa7.appendAll(is.slice(0,chunksize).toTraversable)
          case 7  => sa8.appendAll(is.slice(0,chunksize).toTraversable)
          case 8  => sa9.appendAll(is.slice(0,chunksize).toTraversable)
          case 9  => sa10.appendAll(is.slice(0,chunksize).toTraversable)
          case 10  => sa11.appendAll(is.slice(0,chunksize).toTraversable)
          case 11  => sa12.appendAll(is.slice(0,chunksize).toTraversable)
          case 12  => sa13.appendAll(is.slice(0,chunksize).toTraversable)
          case 13  => sa14.appendAll(is.slice(0,chunksize).toTraversable)
          case 14  => sa15.appendAll(is.slice(0,chunksize).toTraversable)
          case 15  => sa16.appendAll(is.slice(0,chunksize).toTraversable)
          // catch the default with a variable so you can print it
          case default  => ArrayBuffer[(String)]()
        }

      }

    def getsuf(lb: Int) : Int = {

      val chunk = lb/(sa1.length)
      chunk match {
        case 0  => return sa1(lb-chunk*sa1.length).toInt
        case 1  => return sa2(lb-chunk*sa1.length).toInt
        case 2  => return sa3(lb-chunk*sa1.length).toInt
        case 3  => return sa4(lb-chunk*sa1.length).toInt
        case 4  => return sa5(lb-chunk*sa1.length).toInt
        case 5  => return sa6(lb-chunk*sa1.length).toInt
        case 6  => return sa7(lb-chunk*sa1.length).toInt
        case 7  => return sa8(lb-chunk*sa1.length).toInt
        case 8  => return sa9(lb-chunk*sa1.length).toInt
        case 9  => return sa10(lb-chunk*sa1.length).toInt
        case 10  => return sa11(lb-chunk*sa1.length).toInt
        case 11  => return sa12(lb-chunk*sa1.length).toInt
        case 12  => return sa13(lb-chunk*sa1.length).toInt
        case 13  => return sa14(lb-chunk*sa1.length).toInt
        case 14  => return sa15(lb-chunk*sa1.length).toInt
        case 15  => return sa16(lb-chunk*sa1.length).toInt
        // catch the default with a variable so you can print it
        case default  => return 0
      }

    }

    def getref(lb: Int) : Char = {

      if(lb>=ref.length)
        return "N"(0)
      ref(lb)

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
        val nf = new DecimalFormat("#0000000000")
        val fname = nf.format(id) //.toString.split("/")

        fos = fis.create(new Path(hdfsout+"/" + fname+".lz"))
      } catch {
        case e: IOException =>
        //e.printStackTrace()
      }
    //var sampleid = 0
    group.foreach{sample =>
      //println("GROUP: "+x._2+" "+x._3.length+" "+x._4+" REFL: "+ reflength)
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

        try {
          fos.write(posBytes)
          fos.write(lenBytes)
        }catch {
          case e: NullPointerException =>
          e.printStackTrace()
        }
      }

      //sampleid+=1
    }
      fos.close()
      //val delref = new File(reffile).delete()
      //      val delrad = new File(radixout).delete()
    }

    spark.stop()

  }
}

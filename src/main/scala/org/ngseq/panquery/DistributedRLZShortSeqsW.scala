package org.ngseq.panquery

import java.io.{File, IOException, PrintWriter}
import java.net.URI
import java.nio.ByteBuffer
import java.text.DecimalFormat

import org.apache.hadoop.conf.Configuration
import org.apache.hadoop.fs.{FSDataOutputStream, FileSystem, Path}
import org.apache.spark.sql.SparkSession

import scala.collection.mutable.ArrayBuffer
import scala.sys.process.Process
import scala.util.control.Breaks.{break, breakable}

object DistributedRLZShortSeqsW{

  def main(args: Array[String]) {

    val spark = SparkSession.builder.appName("DRLZ").getOrCreate()

    val nf2 = new DecimalFormat("#0000")
    val dataPath = args(0)
    val hdfsurl = args(1)
    val hdfsout = args(2)
    val refsize = args(3).toInt
    val sasplitsize  = args(4).toInt
    val numpart  = args(5).toInt
    //val refsplitsize  = args(6).toInt
    val radixSA = "/opt/chic/radixSA"

    val localOut = "/mnt/tmp/radixout"

    println("Load and preprocess pan-genome")
    //spark.sparkContext.hadoopConfiguration.set("textinputformat.record.delimiter",">")
    //val conf = new Configuration(spark.sparkContext.hadoopConfiguration)
    //conf.set("textinputformat.record.delimiter", ">")

    //println("sorting")
    //val sorted = Process("sort -n " + localOut).lineStream

    println("Load data")

    val splitted = spark.sparkContext.wholeTextFiles(dataPath,numpart)
      .flatMap{rec=>
      //val header = rec._2.substring(1,rec._2.indexOf(System.lineSeparator))
        val split = rec._2.split(">")
        val seqs = ArrayBuffer[(String,Int,String)]()
        System.out.println(rec._1)

        for(i <- 0 to split.length){
          var seq = ""
          if(rec._2.length>0)
          //if(rec._2.indexOf(System.lineSeparator) > 0)
            seq = rec._2.substring(rec._2.indexOf(System.lineSeparator)).trim
          seqs.append((rec._1+"_"+nf2.format(i),seq.length,seq))

        }
        seqs

    }

    //println("Divided pan-genome to "+splitted.count+" ")
    println("Started distributed RLZ")

      val refs = splitted.take(refsize)
      val reffile = "/mnt/tmp/ref.fa"
      val pw = new PrintWriter(new File(reffile))
      refs.foreach(x => pw.write(x._3))
      pw.close()
      val ref = scala.io.Source.fromFile(reffile).getLines().mkString
      val reflength = ref.length
      var radixparams = ""
      if(reflength>800000000)
        radixparams = "-w "
      val radixout = localOut
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

    val SAbc1 = spark.sparkContext.broadcast(sa1)
    val SAbc2 = spark.sparkContext.broadcast(sa2)
    val SAbc3 = spark.sparkContext.broadcast(sa3)
    val SAbc4 = spark.sparkContext.broadcast(sa4)
    val SAbc5 = spark.sparkContext.broadcast(sa5)
    val SAbc6 = spark.sparkContext.broadcast(sa6)
    val SAbc7 = spark.sparkContext.broadcast(sa7)
    val SAbc8 = spark.sparkContext.broadcast(sa8)
    val SAbc9 = spark.sparkContext.broadcast(sa9)
    val SAbc10 = spark.sparkContext.broadcast(sa10)
    val SAbc11 = spark.sparkContext.broadcast(sa11)
    val SAbc12 = spark.sparkContext.broadcast(sa12)
    val SAbc13 = spark.sparkContext.broadcast(sa13)
    val SAbc14 = spark.sparkContext.broadcast(sa14)
    val SAbc15 = spark.sparkContext.broadcast(sa15)
    val SAbc16 = spark.sparkContext.broadcast(sa16)

    val refBC = spark.sparkContext.broadcast(ref)


    def getsuf(lb: Int) : Int = {

      val chunk = lb/(SAbc1.value.length)
      chunk match {
        case 0  => return SAbc1.value(lb-chunk*SAbc1.value.length).toInt
        case 1  => return SAbc2.value(lb-chunk*SAbc1.value.length).toInt
        case 2  => return SAbc3.value(lb-chunk*SAbc1.value.length).toInt
        case 3  => return SAbc4.value(lb-chunk*SAbc1.value.length).toInt
        case 4  => return SAbc5.value(lb-chunk*SAbc1.value.length).toInt
        case 5  => return SAbc6.value(lb-chunk*SAbc1.value.length).toInt
        case 6  => return SAbc7.value(lb-chunk*SAbc1.value.length).toInt
        case 7  => return SAbc8.value(lb-chunk*SAbc1.value.length).toInt
        case 8  => return SAbc9.value(lb-chunk*SAbc1.value.length).toInt
        case 9  => return SAbc10.value(lb-chunk*SAbc1.value.length).toInt
        case 10  => return SAbc11.value(lb-chunk*SAbc1.value.length).toInt
        case 11  => return SAbc12.value(lb-chunk*SAbc1.value.length).toInt
        case 12  => return SAbc13.value(lb-chunk*SAbc1.value.length).toInt
        case 13  => return SAbc14.value(lb-chunk*SAbc1.value.length).toInt
        case 14  => return SAbc15.value(lb-chunk*SAbc1.value.length).toInt
        case 15  => return SAbc16.value(lb-chunk*SAbc1.value.length).toInt
        // catch the default with a variable so you can print it
        case default  => return 0
      }

    }

    def getref(lb: Int) : Char = {

      if(lb>=refBC.value.length)
        return "N"(0)
      refBC.value(lb)

    }

    //val SA.value_tmp = Array(9, 4, 8, 6, 2, 3, 7, 5, 1).map(_ - 1).zipWithIndex.sortBy(_._1)
    //val SA.value = HashMap(SA.value_tmp: _*)

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
    splitted.foreach{sample =>
      //println("GROUP: "+x._2+" "+x._3.length+" "+x._4+" REFL: "+ reflength)
      val encodings = encode(sample._3)

      var fos: FSDataOutputStream = null
      val fis = FileSystem.get(new URI(hdfsurl),new Configuration())
      try {
        //val nf = new DecimalFormat("#0000")
        val fname = sample._1.substring(sample._1.lastIndexOf("/")) //.toString.split("/")
        //fos = fis.create(new Path(hdfsout+"/" + fname.replaceAll(" ","").replaceAll(":","-").replaceAll(",","-")))
        fos = fis.create(new Path(hdfsout+"/" + fname+".lz"))

      } catch {
        case e: IOException =>
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
            //e.printStackTrace()
        }
      }
      fos.close()

      //sampleid+=1
    }

    //val encoded = splitted.map(x => ((x._1,x._2),encode(x._3,reference,SA)))
    //splitted.map(x => ((x._1,x._2),x._3)).sortBy(_._1).map(_._2).saveAsTextFile("real")

    //compress reference using LZ77

    //turbofix to remove '\n'. Currently the algorithm does not seem to write out of ref alphabet
    //chars properly TODO!!
    //val fix = Process("truncate -s-1 " + localIn).!

    //val LZ7 = new LZ77()
    //val refLZ = LZ7.compress(ref._2)

    // order so that the output is written properly
    //val ordered = encoded.sortBy(_._1).flatMap(_._2)
    //ordered.saveAsTextFile("/yarn/total/")


    spark.stop()

  }
}

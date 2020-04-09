package org.ngseq.panquery

import java.io._
import java.net.URI
import java.nio.ByteBuffer

import org.apache.hadoop.conf.Configuration
import org.apache.hadoop.fs.{FSDataOutputStream, FileSystem, Path}
import org.apache.hadoop.hdfs.DFSClient
import org.apache.spark.sql.SparkSession
import xerial.larray.LArray

import scala.collection.mutable.ArrayBuffer
import scala.util.control.Breaks._

object DistributedRLZLarray {


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


    //println("Load suffix")
    //val suffix = scala.io.Source.fromFile(localradix).getLines.toArray.map(_.toInt)

    println("broadcasting")
    //val SA = spark.sparkContext.broadcast(suffix)

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

    //TODO:splittaa data valmiiksi HDFSään(eri kansioihin) ja lue splitin pituus tiedostosta
    //sed -i 's/./&\n/200;s/./&\n/500' ./t

    /*val splitted = data.zipWithIndex.flatMap{x =>
      val fileName = x._1._1.split("/").last
      val groups = x._1._2.grouped(x._1._2.length()/numSplits).toArray
      //.zipWithIndex.map(y => (fileName,y._2,if(y._2==numSplits) y._1+"\n" else y._1))
      //  .zipWithIndex.map(y => (fileName,y._2,y._1))
      //val tmp = groups(groups.length-1)
      //groups(groups.length-1) += "\n"
      val groupz = groups.zipWithIndex.map(y => (fileName,y._2,x._2,y._1))
      groupz
    }.repartition((pgrefs*numSplits))*/

    // call radixSA.value script to generate a file containing the
    // suffix array for parsedRef
    // more flexible
    //TODO: lue vain n ensimmäistä suoraan HDFS:Stä (aja Radix suoraan bash scriptistä n ensimmäiselle jo ennen DRLZtaa)


    // write ref to local
   //println("writing local")
    /*val pw = new PrintWriter(new File(localIn))
    //pw.write(">"+ref._1.split("/").last+"\n")
    nref.foreach(x => pw.write(x._2))
    pw.close()
    val createSA = Process(radixSA + " " + localIn + " " + localOut).!*/
    //println("sorting")
    //val sorted = Process("sort -n " + localOut).lineStream

    //TODO:Suffix for one HG is 29GB!Put suffix to HDFS and read from there
    //TODO: compress suffix array and create reader(maybe BGZF+tabix? or LZ or create db index in HDFS)
    //println("removing")
    //val removed = new File(localOut).delete()

    // load the output
    println("loading to spark")
    // create a stream
    //val SA_tmp = for(i <- sorted) yield {
    //  val xSplit = i.split(" ")
    //(xSplit(0).toInt,xSplit(1).toInt)
    //  xSplit(1).toInt
    //}

    // create a hash table
    //println("creating hashmap")
    //val SA_hash = HashMap(SA_tmp: _*)
    // broadcast the hash table
    // broadcast plain ref (needed for pattern matching)
    //val parsedRef = spark.sparkContext.broadcast(">"+ref._1.split("/").last+"\n"+ref._2)
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
    def binarySearch(lb: Int, rb: Int, ref: String, cur: Char, i: Int, SA_b: LArray[Int]): Option[(Int, Int)] = {

      var low = lb
      var high = rb
      while (low < high) {
        val mid = low + ((high - low) / 2)
        // get the true position

        val midKey = SA_b(mid) + i

        // different "layers"
        val midValue = if (midKey < ref.length) {
          ref(midKey)
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
      //println("low: " + low)
      //println("----------------")

      // break if key not found

      //println(SA_b(low_res)+" + "+i+ " reflen"+ref.length+ " ref(SA_b(low_res) + i)" + (ref(SA_b(low_res) + i)) + "cur "+ cur)

      if ((SA_b(low_res) + i)>= ref.length || ref(SA_b(low_res) + i) != cur) {
        return None
      }
      high = rb
      while (low < high) {
        val mid = low + ((high - low) / 2)
        val midKey = SA_b(mid) + i
        // different "layers"
        val midValue = if (midKey < ref.length) {
          ref(midKey)
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

      if (SA_b(low) != ref.length - 1 && SA_b(low)+i< ref.length && ref(SA_b(low) + i) != cur) {
        //if(low_res>low-1) {
        //  return Some((low_res,low))
        //}
        return Some((low_res, low - 1))
      }
      Some((low_res, low))
    }

    // lb rb clear
    // start j-i
    // cur substring[j]
    // d ref
    //def refine(lb: Int, rb: Int, start: Int, cur: Char, d: String): (Int, Int) = {
    //  val (lb_new,rb_new) = binarySearch(d.slice(SA.value(start,d.length())),cur)
    //}

    // check newline to deal with partition borders (stop phrase search if goes
    // to newline
    def factor(i: Int, split: String, ref:String, SA_b: LArray[Int]): (String, Long) = {

      var lb = 0
      var rb = ref.length-1 // check suffix array size
      var j = i
      breakable {
        while (j < split.length()) {
          //println("j: " + j + " SA.value: " + SA.value(lb))
          //println((SA.value(lb)+j-i) + " " + d.length())

          /*if((SA_b(lb)+j-i) >= d.length()) {
            //println("breaking")
            //break
          }*/
          if (lb == rb && ref(SA_b(lb) + j - i) != split(j)) {
            break
          }
          //(lb,rb) = refine(lb,rb,j-i,x(j))
          val tmp = binarySearch(lb, rb, ref, split(j), j - i,SA_b)
          //println(tmp)

          // perhaps needs more rules
          if (tmp == None) {
            break
          } else {
            //println("jassoo")
            val tmp_tuple = tmp.get
            lb = tmp_tuple._1
            rb = tmp_tuple._2
            //println("jassoo"+lb+","+rb)
            // break if the interval is zero
            // we have found a solution
            /*
            if (lb == rb) {
              println("meni")
              val true_pos = SA.value(lb)
              var k = 1
              while(true_pos+k<d.length()) {
                println("-k-")
                if(d(true_pos + k) != x(j+k)) {
                  j += k
                  break
                } else {
                  k += 1
                }
              }
              break
            }*/
          }
          j += 1
          // border
          if (j == split.length()) {
            break
          }
        }
      }
      //println("out")
      if (j == i) {
        return (split(j).toString(), 0)
      } else {
        //println(i+","+j)
        return (SA_b(lb).toString(), j - i)
      }
    }

    // encode a single substring x
    // finds the longest possible match and returns
    // (pos,len) pair(s)
    def encode(split: String): ArrayBuffer[(String, Long)] = {
      var i: Int = 0
      val max = Int.MaxValue
      val output = ArrayBuffer[(String, Long)]()

      val client = new DFSClient(URI.create(hdfsurl), new Configuration())
      var radixstream = client.open(localradix)
      val bfr = new BufferedReader(new InputStreamReader(radixstream))

      val SA_b =  LArray.of[Int](reference.value.length)

      var line: String = bfr.readLine()

      var l = 0L
      while (line != null) {
        SA_b(l)=java.lang.Integer.valueOf(line.stripLineEnd)
        line = bfr.readLine()
        l+=1
      }

      while (i < split.length) {
        //println(i)

        val tup = factor(i, split, reference.value, SA_b)
        //println("<<<<<<<\n"+tup+"\n<<<<<<<")
        //println("<<<<<<<\n"+tup+"\n<<<<<<<")
        output += tup
        if (tup._2 == 0) {
          i += 1
        } else {
          if(i+tup._2>=max) {
            i = split.length()
          } else {
            i += tup._2.toInt
          }
        }
      }

      SA_b.free

      return output
    }
    println("started encoding")
    //val rsize = (ref._2.length()).toString
    //val nonParsedRef = splitted.filter(x => !nref.map(_._1.split("/").last).contains(x._1))
    //val maxSplit = nonParsedRef.map(_._2).max()
    /*val nonParsedRef = spark.read.text(splitPath)
      .select(org.apache.spark.sql.functions.input_file_name, $"value")
      .as[(String, String)]
      .rdd.groupBy(g=>g._1).zipWithIndex.flatMap{v=>
      //val groups = v.grouped(x._1._2.length()/numSplits).toArray
      //groups.zipWithIndex.map(y => (fileName,y._2,x._2,y._1))
      v._1._2.map(y => (y._1,v._2,y._2.length,y._2))
    }*/


    splitted.foreach{x =>
      //val client = new DFSClient(URI.create("hdfs://m1.novalocal:8020"), new Configuration())
      //val SA = client.open("/user/root/radixout.txt")

      val encodings = encode(x._3)
      //if(x._2==0) {
      //val newLine = (rsize,1L)
      //encodings.prepend(("\n",0))
      //encodings += newLine
      //encodings += ("\n",0)
      //} else if(x._2 == 0) {
      //add fasta
      //encodings.prependAll(encode("\n>"+x._1+"\n",parsedRef,SA))
      //}
      var fos: FSDataOutputStream = null
      val fis = FileSystem.get(new URI(hdfsurl),new Configuration())

      try {
        //val nf = new DecimalFormat("#0000000")
        val fname = x._1.toString.split("/")

        fos = fis.create(new Path(hdfsout+"/" + fname(fname.length-1)+".lz"))
      } catch {
        case e: IOException =>
        //e.printStackTrace()
      }

      encodings.foreach{z =>
        //println(x._1+","+x._2)
        var posBytes: Array[Byte] = null

        val len = z._2
        if(len != 0) {
          posBytes = ByteBuffer.allocate(8).putLong(z._1.toLong).array.reverse
          //posBytes = x._1.getBytes
        }
        else {
          //posBytes = ByteBuffer.allocate(8).putLong(rsize).array.reverse
          //len = 1
          posBytes = ByteBuffer.allocate(8).putLong(z._1(0).toLong).array.reverse
          //posBytes = x._1(0).toString.getBytes
        }
        val lenBytes = ByteBuffer.allocate(8).putLong(len).array.reverse
        //(posBytes,lenBytes)
        //val lenBytes = len.toString.getBytes

        //println(x._1,len)
        fos.write(posBytes)
        fos.write(lenBytes)
    }
    //val encoded = splitted.map(x => ((x._1,x._2),encode(x._3,parsedRef,SA)))
    //splitted.map(x => ((x._1,x._2),x._3)).sortBy(_._1).map(_._2).saveAsTextFile("real")

    //compress parsedRef using LZ77

    //turbofix to remove '\n'. Currently the algorithm does not seem to write out of ref alphabet
    //chars properly TODO!!
    //val fix = Process("truncate -s-1 " + localIn).!


      /*try
        fos.write(baos)
      catch {
        case e: IOException =>
          e.printStackTrace()
      }*/
     //baos.close()
     fos.close()

    }

    /*.toLocalIterator

    val bos = new BufferedOutputStream(new FileOutputStream(output,true))

    while(local.hasNext) {
      //TODO: this is slow and spends memory! Write in map stage to numbered partitions as with Sparkbeagle and getmerge etc.
      val tmp = local.next
      bos.write(tmp._1)
      bos.write(tmp._2)
    }
    bos.close()*/

    //println(ordered.map{x =>
    //  if(x._2 == 0) x._1.toString else parsedRef.value.slice(x._1.toInt,x._1.toInt+x._2.toInt).toString
    //}.collect().foldLeft("")(_+_))

    spark.stop()

  }
}

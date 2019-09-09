package org.ngseq.panquery

import java.io._
import java.nio.ByteBuffer

import org.apache.spark.broadcast.Broadcast
import org.apache.spark.sql.SparkSession
import org.apache.spark.sql.functions.input_file_name

import scala.collection.mutable.ArrayBuffer
import scala.sys.process._
import scala.util.control.Breaks._

object DistributedRLZ {

  def main(args: Array[String]) {

    val spark = SparkSession.builder.appName("DRLZ").getOrCreate()
    import spark.implicits._

    val dataPath = args(0)
    val refSize = args(1).toInt // number of refs
    val numSplits = args(2).toInt // control how many splits we want for the data

    val radixSA = "./radixSA"
    val localIn = "radixin.txt"
    val localOut = "radixout.txt"
    val refParse = "./rlz_for_hybrid"
    val output = "merged.lz"
    // download the data that is generated using vcf multialign
    // get the file name (patient name) for fasta flag

    val data = spark.read.text(dataPath)
      .select(input_file_name, $"value")
      .as[(String, String)]
      .rdd
    //val data = spark.sparkContext.wholeTextFiles(dataPath)//.sortBy(_._1)

    val size = data.count()

    // now hard coded ref
    //val ref = data.take(1)//.slice(10000,150000)

    // more flexible
    val nref = data.take(refSize)


    val splitted = data.zipWithIndex.flatMap{x =>
      val fileName = x._1._1.split("/").last
      val groups = x._1._2.grouped(x._1._2.length()/numSplits).toArray
      //.zipWithIndex.map(y => (fileName,y._2,if(y._2==numSplits) y._1+"\n" else y._1))
      //  .zipWithIndex.map(y => (fileName,y._2,y._1))
      //val tmp = groups(groups.length-1)
      //groups(groups.length-1) += "\n"
      groups.zipWithIndex.map(y => (fileName,y._2,x._2,y._1))
    }.repartition((size*numSplits).toInt)

    // call radixSA.value script to generate a file containing the
    // suffix array for reference

    // write ref to local
    println("writing local")
    val pw = new PrintWriter(new File(localIn))
    //pw.write(">"+ref._1.split("/").last+"\n")
    nref.foreach(x => pw.write(x._2))
    pw.close()

    val createSA = Process(radixSA + " " + localIn + " " + localOut).!
    //println("sorting")
    //val sorted = Process("sort -n " + localOut).lineStream

    println("Load suffix")
    val suffix = scala.io.Source.fromFile(localOut).getLines.toArray.map(_.toInt)

    println("removing")
    val removed = new File(localOut).delete()

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
    println("broadcasting")
    val SA = spark.sparkContext.broadcast(suffix)

    // broadcast plain ref (needed for pattern matching)
    //val reference = spark.sparkContext.broadcast(">"+ref._1.split("/").last+"\n"+ref._2)
    val reference = spark.sparkContext.broadcast(nref.map(_._2).mkString(""))

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
    def binarySearch(lb: Int, rb: Int, d_b: Broadcast[String], cur: Char, i: Int, SA_b: Broadcast[Array[Int]]): Option[(Int, Int)] = {
      val d = d_b.value
      var low = lb
      var high = rb
      while (low < high) {
        val mid = low + ((high - low) / 2)
        // get the true position
        val midKey = SA_b.value(mid) + i

        // different "layers"
        val midValue = if (midKey < d.length()) {
          d(midKey)
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
      if ((SA_b.value(low_res) + i)>= d.length || d(SA_b.value(low_res) + i) != cur) {
        return None
      }
      high = rb
      while (low < high) {
        val mid = low + ((high - low) / 2)
        val midKey = SA_b.value(mid) + i
        // different "layers"
        val midValue = if (midKey < d.length()) {
          d(midKey)
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
      if (SA_b.value(low) != d.length() - 1 && SA_b.value(low)+i< d.length() && d(SA_b.value(low) + i) != cur) {
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
    def factor(i: Int, x: String, d_b: Broadcast[String], SA_b: Broadcast[Array[Int]]): (String, Long) = {
      val d = d_b.value
      var lb = 0
      var rb = d.length()-1 // check suffix array size
      var j = i
      breakable {
        while (j < x.length()) {
          //println("j: " + j + " SA.value: " + SA.value(lb))
          //println((SA.value(lb)+j-i) + " " + d.length())
          if((SA_b.value(lb)+j-i) >= d.length()) {
            //println("breaking")
            //break
          }
          if (lb == rb && d(SA_b.value(lb) + j - i) != x(j)) {
            break
          }
          //(lb,rb) = refine(lb,rb,j-i,x(j))
          val tmp = binarySearch(lb, rb, d_b, x(j), j - i,SA_b)
          //println(tmp)

          // perhaps needs more rules
          if (tmp == None) {
            break
          } else {
            //println("jassoo")
            val tmp_tuple = tmp.get
            lb = tmp_tuple._1
            rb = tmp_tuple._2
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

        return (SA_b.value(lb).toString(), j - i)
      }
    }

    // encode a single substring x
    // finds the longest possible match and returns
    // (pos,len) pair(s)
    def encode(x: String, d_b: Broadcast[String],SA_b: Broadcast[Array[Int]]): ArrayBuffer[(String, Long)] = {
      var i: Int = 0
      val max = Int.MaxValue
      val output = ArrayBuffer[(String, Long)]()
      val d = d_b.value
      while (i < x.length()) {
        //println(i)
        val tup = factor(i, x, d_b,SA_b)
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
    //val rsize = (ref._2.length()).toString
    val filteredTmp = splitted.filter(x => !nref.map(_._1.split("/").last).contains(x._1))
    //val maxSplit = filteredTmp.map(_._2).max()
    val encoded = filteredTmp.map{x =>
      val encodings = encode(x._4,reference,SA)
      //if(x._2==0) {
      //val newLine = (rsize,1L)
      //encodings.prepend(("\n",0))
      //encodings += newLine
      //encodings += ("\n",0)
      //} else if(x._2 == 0) {
      //add fasta
      //encodings.prependAll(encode("\n>"+x._1+"\n",reference,SA))
      //}
      ((x._1,x._2,x._3),encodings)
    }
    //val encoded = splitted.map(x => ((x._1,x._2),encode(x._3,reference,SA)))
    //splitted.map(x => ((x._1,x._2),x._3)).sortBy(_._1).map(_._2).saveAsTextFile("real")

    //compress reference using LZ77

    //turbofix to remove '\n'. Currently the algorithm does not seem to write out of ref alphabet
    //chars properly TODO!!
    //val fix = Process("truncate -s-1 " + localIn).!
    val LZ77 = Process(refParse + " " + localIn + " 1 " + localIn + " " + output + " 4 4 5000 0").!
    //val LZ7 = new LZ77()
    //val refLZ = LZ7.compress(ref._2)

    // order so that the output is written properly
    val ordered = encoded.sortBy(_._1).flatMap(_._2)
    //ordered.saveAsTextFile("/yarn/total/")

    // create bytearrays and collect to master via iterator (to prevent driver memory from
    // getting full)
    val local = ordered.map{x=>
      var posBytes: Array[Byte] = null
      var len = x._2
      if(len != 0) {
        posBytes = ByteBuffer.allocate(8).putLong(x._1.toLong).array.reverse
      }
      else {
        //posBytes = ByteBuffer.allocate(8).putLong(rsize).array.reverse
        //len = 1
        posBytes = ByteBuffer.allocate(8).putLong(x._1(0).toLong).array.reverse
      }
      val lenBytes = ByteBuffer.allocate(8).putLong(len).array.reverse
      println(x._1+","+len)
      (posBytes,lenBytes)
    }.toLocalIterator

    val bos = new BufferedOutputStream(new FileOutputStream(output,true))
    //refLZ.foreach(x => bos.write(ByteBuffer.allocate(8).putChar(x).array.reverse))
    while(local.hasNext) {
      //TODO: this is slow and spends memory! Write in map stage to numbered partitions as with Sparkbeagle and getmerge etc.
      val tmp = local.next
      //println(tmp._1+","+tmp._2)
      bos.write(tmp._1)
      bos.write(tmp._2)
    }
    bos.close()
    ordered.saveAsTextFile("out")
    //println(ordered.map{x =>
    //  if(x._2 == 0) x._1.toString else reference.value.slice(x._1.toInt,x._1.toInt+x._2.toInt).toString
    //}.collect().foldLeft("")(_+_))

    spark.stop()

  }
}

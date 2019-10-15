package org.ngseq.panquery

import java.io._
import java.net.URI
import java.nio.ByteBuffer
import java.util

import org.apache.hadoop.conf.Configuration
import org.apache.hadoop.fs.{FSDataOutputStream, FileSystem, Path}
import org.apache.spark.sql.SparkSession

import scala.collection.mutable.ArrayBuffer
import scala.sys.process._
import scala.util.control.Breaks._

object DistributedRLZ2A {


  def main(args: Array[String]) {

    val spark = SparkSession.builder.appName("DRLZ").getOrCreate()

    val refPath = args(0)
    val splitPath = args(1) // number of refsf
    val numSplits = args(2).toInt // control how many splits we want for the data
    val radixFile = args(3)
    val hdfsout = args(4)
    val hdfsurl = args(5)


    //val pgrefs = args(3).toInt //

    val radixSA = "./radixSA"
    val refLocal = "radixin.txt"
    //val radixFile = "radixout.txt"
    val refParse = "./rlz_for_hybrid"
    val output = "merged.lz"
    // download the data that is generated using vcf multialign
    // get the file name (patient name) for fasta flagdata.count()

    /*val data = spark.read.text(pgPath)
      .select(input_file_name, $"value")
      .as[(String, String)]
      .rdd*/
    //val data = spark.sparkContext.wholeTextFiles(pgPath)//.sortBy(_._1)

    // now hard coded ref
    //val ref = data.take(1)//.slice(10000,150000)

    val fs = FileSystem.get(new Configuration())
    val pgFileList = new util.ArrayList[String]

    val st = fs.listStatus(new Path(splitPath))
    st.foreach{s=>
        pgFileList.add(s.getPath.toUri.getRawPath)
    }
    val reffile = pgFileList.get(0)
    val reflength = st(0).getLen
    println("reffile:::::"+refPath)
    println("REFLEN:::::"+reflength)

    println("loading to spark")

    val refdata1 = spark.sparkContext.textFile(refPath+"_r1").collect()
    val refdata2 = spark.sparkContext.textFile(refPath+"_r2").collect()
    val refdata3 = spark.sparkContext.textFile(refPath+"_r3").collect()
    val refdata4 = spark.sparkContext.textFile(refPath+"_r4").collect()


    val radixdata1 = spark.sparkContext.textFile(radixFile+"_r1").collect()
    val radixdata2 = spark.sparkContext.textFile(radixFile+"_r2").collect()
    val radixdata3 = spark.sparkContext.textFile(radixFile+"_r3").collect()
    val radixdata4 = spark.sparkContext.textFile(radixFile+"_r4").collect()

    /*    val client = new DFSClient(URI.create(hdfsurl), new Configuration())
        var radixstream = client.open(radixFile)
        val bfr = new BufferedReader(new InputStreamReader(radixstream))
        var refstream = client.open(reffile)
        val bfref = new BufferedReader(new InputStreamReader(refstream))

        val rflen = (reflength/2).toInt
        val SA_b1 = new ArrayBuffer[Long]
        //val SA_b2 = Array.fill(rf)(-1L)
        var SA_b2 = new ArrayBuffer[Long]
        var line: String = bfr.readLine()

        var cnt = 0L

        while (line != null) {
          if(cnt<rflen)
            SA_b1.append(line.stripLineEnd.toLong)
          else
            SA_b2.append(line.stripLineEnd.toLong)
          cnt+=1
          line = bfr.readLine()
        }

        cnt = 0L

        var ref1 =""
        var ref2 = ""

        var value: Int = bfref.read()
        while ( {value != -1}) {
          val c =value.toChar

          if(cnt<rflen)
            ref1 = ref1+c
          else
            ref2 = ref2+c
          cnt+=1
          value = bfref.read()
        }
    */

    println("broadcasting")
    val rlen = spark.sparkContext.broadcast(reflength)

    val refbc1 = spark.sparkContext.broadcast(refdata1)
    val refbc2 = spark.sparkContext.broadcast(refdata2)
    val refbc3 = spark.sparkContext.broadcast(refdata3)
    val refbc4 = spark.sparkContext.broadcast(refdata4)

    val SAbc1 = spark.sparkContext.broadcast(radixdata1)
    val SAbc2 = spark.sparkContext.broadcast(radixdata2)
    val SAbc3 = spark.sparkContext.broadcast(radixdata3)
    val SAbc4 = spark.sparkContext.broadcast(radixdata4)
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
    def binarySearch(lb: Long, rb: Long, ref: Array[String], cur: Char, i: Int, SA_b: Array[Array[String]]): Option[(Long, Long)] = {

      var low = lb
      var high = rb
      val rlen = ref(0).length
      while (low < high) {
        var mid = low + ((high - low) / 2)
        // get the true position

        //TODO: both arrays needed here, if midkey > high access upper half array
        var midKey = 0L
        var newmid = mid
        if (mid > rlen){
          newmid = mid-rlen
          midKey = SA_b(1)(newmid.toInt).toLong + i
        }
        else
          midKey = SA_b(0)(mid.toInt).toLong + i


        // different "layers"
        val midValue = if (midKey < reflength) {
          if (midKey > rlen)
              ref(1)((midKey-rlen).toInt)
          else
              ref(0)(midKey.toInt)
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
      var low_res = low
      //println("low: " + low)
      //println("----------------")

      // break if key not found

      //println(SA_b(low_res)+" + "+i+ " reflen"+ref.length+ " ref(SA_b(low_res) + i)" + (ref(SA_b(low_res) + i)) + "cur "+ cur)

      //SELECT ARRAY
      var saind = 0
      var newlow_res = low_res
      if (low_res > rlen){
        newlow_res = low_res-rlen
        saind = 1
      }
      val sabsum = SA_b(saind)(newlow_res.toInt).toLong+i
      var referenceIndex = 0
      var baseindex = sabsum
      if (baseindex > rlen){
        referenceIndex = 1
        baseindex = (sabsum-rlen-1)
      }
      //END
      val refbase = ref(referenceIndex)(baseindex.toInt)
      if ( sabsum >= reflength)
        return None
      if ( refbase != cur)
        return None

      high = rb
      while (low < high) {
        var mid = low + ((high - low) / 2)
        var midKey = 0L
        var newmid = mid

        if (mid > rlen){
          newmid = mid-rlen-1
          midKey = SA_b(1)(newmid.toInt).toLong + i
        }
        else
          midKey = SA_b(0)(newmid.toInt).toLong + i


        // different "layers"
        val midValue = if (midKey < reflength) {
          if (midKey > rlen)
            ref(1)((midKey-rlen-1).toInt)
          else
            ref(0)(midKey.toInt)
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

      //SELECT ARRAY
      var sind = 0
      var newlow = low
      if (low > rlen){
        newlow = low-rlen-1
        sind = 1
      }
      val sbs = SA_b(sind)(newlow.toInt).toLong+i
      var refind = 0
      var bind = sbs
      if (bind > rlen){
        refind = 1
        bind = (sbs-rlen-1)
      }
      //END

      val rbase = ref(refind)(bind.toInt)
      if (SA_b(sind)(newlow.toInt).toLong != (reflength - 1) && sbs < reflength && rbase != cur) {
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
    def factor(i: Int, split: String, ref: Array[String], SA_b: Array[Array[String]], reflen: Long): (String, Int) = {

      var lb = 0L
      var rb = reflen-1 // check suffix array size
      var j = i
      var sai = 0
      var lsb = 0
      var rsb = 0
      var rai = 0

      breakable {
        while (j < split.length()) {
          if(lb >= reflen/2){
            lsb = (lb - (reflen/2)-1).toInt
            sai = 1
          }else{
            sai = 0
            lsb = lb.toInt
          }

          if(rb >= reflen/2-1){
            rsb = (rb - (reflen/2)-1).toInt
            rai = 1
          }else{
            rai = 0
            rsb = rb.toInt
          }
          //println("j: " + j + " SA.value: " + SA.value(lb))
          //println((SA.value(lb)+j-i) + " " + d.length())

          /*if((SA_b(lb)+j-i) >= d.length()) {
            //println("breaking")
            //break
          }*/
          if (lb == rb) {
            val sabsum = SA_b(sai)(lsb).toLong + j - i
            var right = 0
            var sabsum_tmp = sabsum
            if (sabsum >= reflen/2){
              right = 1
              sabsum_tmp = sabsum - (reflen/2)
            }
            val ray=ref(right)(sabsum_tmp.toInt)
            val sp = split(j)
            if (ray != sp)
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
        val ret = SA_b(sai)(lsb).toString()
        return (ret, j - i)
      }
    }

    // encode a single substring x
    // finds the longest possible match and returns
    // (pos,len) pair(s)
    def encode(split: String, reflen: Long): ArrayBuffer[(String, Int)] = {
      var i: Int = 0
      val max = Int.MaxValue
      val output = ArrayBuffer[(String, Int)]()



      val SA_b = Array(SAbc1.value,SAbc2.value)
      val ref = Array(refbc1.value(0),refbc2.value(0))

      while (i < split.length) {
        //println(i)

        val tup = factor(i, split, ref, SA_b, reflen)
        //println("<<<<<<<\n"+tup+"\n<<<<<<<")
        output += tup
        if (tup._2 == 0) {
          i += 1
        } else {
          if(i+tup._2>=max) {
            i = split.length()
          } else {
            i += tup._2
          }
        }
      }

      return output
    }


    println("started encoding")
    //val rsize = (ref._2.length()).toString
    //val nonParsedRef = splitted.filter(x => !nref.map(_._1.split("/").last).contains(x._1))
    //val maxSplit = nonParsedRef.map(_._2).max()
    import spark.implicits._
    /*val nonParsedRef = spark.read.text(splitPath)
      .select(org.apache.spark.sql.functions.input_file_name, $"value")
      .as[(String, String)]
      .rdd.groupBy(g=>g._1).zipWithIndex.flatMap{v=>
      //val groups = v.grouped(x._1._2.length()/numSplits).toArray
      //groups.zipWithIndex.map(y => (fileName,y._2,x._2,y._1))
      v._1._2.map(y => (y._1,v._2,y._2.length,y._2))
    }*/

    val nonParsedRef = spark.read.text(splitPath)
      .select(org.apache.spark.sql.functions.input_file_name, $"value")
      .as[(String, String)]
      .rdd.map{v=>
      //val groups = v.grouped(x._1._2.length()/numSplits).toArray
      //groups.zipWithIndex.map(y => (fileName,y._2,x._2,y._1))
     (v._1,v._2.length,v._2)
    }

    val encoded = nonParsedRef.map{x =>
      //val client = new DFSClient(URI.create("hdfs://m1.novalocal:8020"), new Configuration())
      //val SA = client.open("/user/root/radixout.txt")

      val encodings = encode(x._3,rlen.value)
      //if(x._2==0) {
      //val newLine = (rsize,1L)
      //encodings.prepend(("\n",0))
      //encodings += newLine
      //encodings += ("\n",0)
      //} else if(x._2 == 0) {
      //add fasta
      //encodings.prependAll(encode("\n>"+x._1+"\n",parsedRef,SA))
      //}
      ((x._1,x._2,x._3),encodings)
    }
    //val encoded = splitted.map(x => ((x._1,x._2),encode(x._3,parsedRef,SA)))
    //splitted.map(x => ((x._1,x._2),x._3)).sortBy(_._1).map(_._2).saveAsTextFile("real")

    //compress parsedRef using LZ77

    //turbofix to remove '\n'. Currently the algorithm does not seem to write out of ref alphabet
    //chars properly TODO!!
    //val fix = Process("truncate -s-1 " + localIn).!
    val LZ77 = Process(refParse + " " + refLocal + " 1 " + refLocal + " " + output + " 4 4 5000 0").!
    //val LZ7 = new LZ77()
    //val refLZ = LZ7.compress(ref._2)

    // order so that the output is written properly
    val ordered = encoded//.coalesce(numSplits).sortBy(_._1._2)

    // create bytearrays and collect to master via iterator (to prevent driver memory from
    // getting full)
   //encoded.saveAsTextFile(hdfsout)

    ordered.foreach{part=>

      var values=part._2.toArray

      var fos: FSDataOutputStream = null
      val fis = FileSystem.get(new URI(hdfsurl),new Configuration())

     try {
        //val nf = new DecimalFormat("#0000000")
        val fname = part._1._1.toString.split("/")

        fos = fis.create(new Path(hdfsout+"/" + fname(fname.length-1)+".pos"))
      } catch {
        case e: IOException =>
          e.printStackTrace()
      }

      values.foreach{x =>
        //println(x._1+","+x._2)
        var posBytes: Array[Byte] = null

        val len = x._2
        if(len != 0) {
          posBytes = ByteBuffer.allocate(8).putLong(x._1.toLong).array.reverse
          //posBytes = x._1.getBytes
        }
        else {
          //posBytes = ByteBuffer.allocate(8).putLong(rsize).array.reverse
          //len = 1
          posBytes = ByteBuffer.allocate(8).putLong(x._1(0).toLong).array.reverse
          //posBytes = x._1(0).toString.getBytes
        }
        val lenBytes = ByteBuffer.allocate(8).putLong(len).array.reverse
        //(posBytes,lenBytes)
        //val lenBytes = len.toString.getBytes

        //println(x._1,len)
        fos.write(posBytes)
        fos.write(lenBytes)

      }

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

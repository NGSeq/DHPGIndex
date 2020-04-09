import java.io._
import java.net.URI
import java.nio.ByteBuffer
import java.text.DecimalFormat

import org.apache.hadoop.conf.Configuration
import org.apache.hadoop.fs.{FSDataOutputStream, FileSystem, Path}
import org.apache.spark.sql.SparkSession

import scala.collection.mutable.ArrayBuffer
import scala.sys.process.Process
import scala.util.control.Breaks._

object DistributedRLZSpart {

  def main(args: Array[String]) {

    val spark = SparkSession.builder.appName("DRLZ").getOrCreate()
    import spark.implicits._

    val chr = args(0).toInt
    val dataPath = args(1)
    val hdfsurl = args(2)
    val hdfsout = args(3)
    val refsize = args(4).toInt
    val sasplitsize  = args(5).toInt
    //val refsplitsize  = args(6).toInt
    val chunks = args(6).toInt
    val radixSA = "/mnt/tmp/radixSA"

    val localOut = "/mnt/tmp/radixout."+chr
    val refParse = "./rlz_for_hybrid"
    val output = "merged.lz"


    // broadcast plain ref (needed for pattern matching)
    //val reference = spark.sparkContext.broadcast(">"+ref._1.split("/").last+"\n"+ref._2)


    //println("sorting")
    //val sorted = Process("sort -n " + localOut).lineStream

    println("Load suffix")
    //val suffix = scala.io.Source.fromFile(localOut).getLines.toArray.map(_.toInt)
    val splitted = spark.read.text(dataPath).select(org.apache.spark.sql.functions.input_file_name, $"value")
       .as[(String, String)]
       .rdd.flatMap{v=>
       val gapless = v._2.replaceAll("-", "")
       val groups = gapless.grouped(v._2.length/chunks).toArray
       groups.zipWithIndex.map(y => (v._1,y._1.length,y._1,y._2))

     }.groupBy(g => g._4)

    println("GROUPSCNT"+splitted.getNumPartitions)
    splitted.foreach{group=>
      println("GROUPNUM:"+group._1 )
      println("SEQSinGROUP:"+group._2.toArray.length)
      val refs = group._2.take(refsize)
      val reffile = "/mnt/tmp/ref"+chr+"_"+group._1
      val pw = new PrintWriter(new File(reffile))
      refs.foreach(x => pw.write(x._3))
      pw.close()
      val ref = scala.io.Source.fromFile(reffile).getLines().mkString
      val reflength = ref.length
      println("reflength"+reflength)
      var radixparams = ""
      if(reflength>800000000)
        radixparams = "-w "
      val radixout = localOut+"_"+group._1
      val createSA = Process(radixSA + " "+radixparams+" "+reffile+" " + radixout).!
      /*val split = Process("./split.sh "+localOut+" "+sasplitsize+ " chr"+chr+"_"+group._1).!
      val splitref = Process("./splitref.sh "+reffile+ " " +refsplitsize+ " ref"+chr+"_"+group._1).!
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

      /* var ref1 = ""
    var ref2 = ""
    var ref3 = ""
    var ref4 = ""
    var ref5 = ""
    var ref6 = ""
    var ref7 = ""
    var ref8 = ""
    var ref9 = ""
    var ref10 = ""
    var ref11 = ""
    var ref12 = ""
    var ref13 = ""
    var ref14 = ""
    var ref15 = ""
    var ref16 = ""*/
      
      val is = scala.io.Source.fromFile(radixout).getLines()

      val chunksize = (reflength/sasplitsize)+1
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

    /*for(j <- 0 to refsplitsize-1){

      j match {
        case 0  => ref1=scala.io.Source.fromFile("x00.ref"+chr).getLines.mkString
        case 1  => ref2=scala.io.Source.fromFile("x01.ref"+chr).getLines.mkString
        case 2  => ref3=scala.io.Source.fromFile("x02.ref"+chr).getLines.mkString
        case 3  => ref4=scala.io.Source.fromFile("x03.ref"+chr).getLines.mkString
        case 4  => ref5=scala.io.Source.fromFile("x04.ref"+chr).getLines.mkString
        case 5  => ref6=scala.io.Source.fromFile("x05.ref"+chr).getLines.mkString
        case 6  => ref7=scala.io.Source.fromFile("x06.ref"+chr).getLines.mkString
        case 7  => ref8=scala.io.Source.fromFile("x07.ref"+chr).getLines.mkString
        case 8  => ref9=scala.io.Source.fromFile("x08.ref"+chr).getLines.mkString
        case 9  => ref10=scala.io.Source.fromFile("x09.ref"+chr).getLines.mkString
        case 10  => ref11=scala.io.Source.fromFile("x10.ref"+chr).getLines.mkString
        case 11  => ref12=scala.io.Source.fromFile("x11.ref"+chr).getLines.mkString
        case 12  => ref13=scala.io.Source.fromFile("x12.ref"+chr).getLines.mkString
        case 13  => ref14=scala.io.Source.fromFile("x13.ref"+chr).getLines.mkString
        case 14  => ref15=scala.io.Source.fromFile("x14.ref"+chr).getLines.mkString
        case 15  => ref16=scala.io.Source.fromFile("x15.ref"+chr).getLines.mkString
        // catch the default with a variable so you can print it
        case default  => ""
      }

    }*/

   /* val SAbc1 = spark.sparkContext.broadcast(sa1)
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


    val Refbc1 = spark.sparkContext.broadcast(ref1)
    val Refbc2 = spark.sparkContext.broadcast(ref2)
    val Refbc3 = spark.sparkContext.broadcast(ref3)
    val Refbc4 = spark.sparkContext.broadcast(ref4)
    val Refbc5 = spark.sparkContext.broadcast(ref5)
    val Refbc6 = spark.sparkContext.broadcast(ref6)
    val Refbc7 = spark.sparkContext.broadcast(ref7)
    val Refbc8 = spark.sparkContext.broadcast(ref8)
    val Refbc9 = spark.sparkContext.broadcast(ref9)
    val Refbc10 = spark.sparkContext.broadcast(ref10)
    val Refbc11 = spark.sparkContext.broadcast(ref11)
    val Refbc12 = spark.sparkContext.broadcast(ref12)
    val Refbc13 = spark.sparkContext.broadcast(ref13)
    val Refbc14 = spark.sparkContext.broadcast(ref14)
    val Refbc15 = spark.sparkContext.broadcast(ref15)
    val Refbc16 = spark.sparkContext.broadcast(ref16)*/

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

     /* val chnk = lb/(Refbc1.value.length)
      chnk match {
        case 0  => return Refbc1.value(lb-chnk*Refbc1.value.length)
        case 1  => return Refbc2.value(lb-chnk*Refbc1.value.length)
        case 2  => return Refbc3.value(lb-chnk*Refbc1.value.length)
        case 3  => return Refbc4.value(lb-chnk*Refbc1.value.length)
        case 4  => return Refbc5.value(lb-chnk*Refbc1.value.length)
        case 5  => return Refbc6.value(lb-chnk*Refbc1.value.length)
        case 6  => return Refbc7.value(lb-chnk*Refbc1.value.length)
        case 7  => return Refbc8.value(lb-chnk*Refbc1.value.length)
        case 8  => return Refbc9.value(lb-chnk*Refbc1.value.length)
        case 9  => return Refbc10.value(lb-chnk*Refbc1.value.length)
        case 10  => return Refbc11.value(lb-chnk*Refbc1.value.length)
        case 11  => return Refbc12.value(lb-chnk*Refbc1.value.length)
        case 12  => return Refbc13.value(lb-chnk*Refbc1.value.length)
        case 13  => return Refbc14.value(lb-chnk*Refbc1.value.length)
        case 14  => return Refbc15.value(lb-chnk*Refbc1.value.length)
        case 15  => return Refbc16.value(lb-chnk*Refbc1.value.length)
        // catch the default with a variable so you can print it
        case default  => return "N"(0)
      }*/
      if(lb>=ref.length)
        return "N"(0)
      ref(lb)

    }

    //val createSA = Process(radixSA + " " + localref + " " + localOut).!
    //println("sorting")
    //val sorted = Process("sort -n " + localOut).lineStream

    // load the output
    println("loaded to spark")
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
      //println("low: " + low)
      //println("----------------")

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
    def factor(i: Int, x: String): (String, Long) = {

      var lb = 0
      var rb = reflength-1 // check suffix array size
      var j = i
      breakable {
        while (j < x.length()) {
          //println("j: " + j + " SA.value: " + SA.value(lb))
          //println((SA.value(lb)+j-i) + " " + d.length())
          if((getsuf(lb)+j-i) >= reflength) {
            //println("breaking")
            //break
          }
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
    //val rsize = (ref._2.length()).toString

    //val maxSplit = filteredTmp.map(_._2).max()
    var partid = 0
    group._2.foreach{x =>
      println("XXX: "+x._2+" "+x._3.length+" "+x._4+" REFL: "+ reflength)
      val encodings = encode(x._3)
      //if(x._2==0) {
      //val newLine = (rsize,1L)
      //encodings.prepend(("\n",0))
      //encodings += newLine
      //encodings += ("\n",0)
      //} else if(x._2 == 0) {
      //add fasta
      //encodings.prependAll(encode("\n>"+x._1+"\n",reference,SA))
      //}
      var fos: FSDataOutputStream = null
      val fis = FileSystem.get(new URI(hdfsurl),new Configuration())
      try {
        val nf = new DecimalFormat("#0000")
        val fname = nf.format(chr)+"_"+nf.format(partid)+"_"+nf.format(group._1) //.toString.split("/")

        fos = fis.create(new Path(hdfsout+"/" + fname+".lz"))
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
      fos.close()

      partid+=1
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

    }
    spark.stop()

  }
}

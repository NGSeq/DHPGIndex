package org.ngseq.dhpgidx

import java.io._
import java.net.URI
import java.util.UUID

import org.apache.hadoop.conf.Configuration
import org.apache.hadoop.fs.{FileSystem, Path}
import org.apache.hadoop.hdfs.DFSClient
import org.apache.hadoop.io.{LongWritable, Text}
import org.apache.hadoop.mapreduce.lib.input.TextInputFormat
import org.apache.spark.sql.SparkSession

object DistributedIndexing{

  def main(args: Array[String]) {

    val spark = SparkSession.builder.appName("DRLZ").getOrCreate()

    val dataPath = args(0)
    val hdfsurl = args(1)
    val hdfsout = args(2)
    val chunks = args(3).toInt
    val maxqlen = args(4).toInt

    println("Load and preprocess pan-genome")
    spark.sparkContext.hadoopConfiguration.set("textinputformat.record.delimiter",">")
    val conf = new Configuration(spark.sparkContext.hadoopConfiguration)
    conf.set("textinputformat.record.delimiter", ">")

    val data = spark.sparkContext.newAPIHadoopFile(dataPath, classOf[TextInputFormat], classOf[LongWritable], classOf[Text], conf).filter(x=>x._2.getLength!=0)

    data.coalesce(chunks).foreachPartition{part =>
      val client = new DFSClient(URI.create(hdfsurl), new Configuration())
      val id = UUID.randomUUID()
      val bos = new BufferedOutputStream(new FileOutputStream("/mnt/tmp/"+id+".lz",true))

      //var fis: FSDataInputStream = null
      val fs = FileSystem.get(new URI(hdfsurl),new Configuration())

      val pw = new PrintWriter(new File("/mnt/tmp/"+id+".fa"))

      while(part.hasNext){
        val record = part.next()._2.toString
        pw.write(">"+record)
        val fstatus = fs.globStatus(new Path(hdfsurl+"/"+hdfsout+"/"+record.split(System.lineSeparator())(0)+"*"))
        fstatus.foreach{f=>
          System.out.println(f.getPath.getName)
          val dfsInputStream = client.open(hdfsout+"/"+f.getPath.getName)
          val isr = new BufferedReader(new InputStreamReader(dfsInputStream))

          var byte: Int = 0
          while ({byte = isr.read(); byte != -1}) {
            bos.write(byte)
          }
          dfsInputStream.close()
        }

      }
      pw.close()
      bos.close()

      val chicproc = new ProcessBuilder("/bin/bash", "-c", "/opt/chic/src/chic_index --threads=16  --kernel=BLAST --verbose=2 --lz-input-file=/mnt/tmp/"+id+".lz -o /mnt/tmp/"+id+".idx /mnt/tmp/"+id+".fa "+maxqlen)
      val chic = chicproc.start()
      val err = new BufferedReader(new InputStreamReader(chic.getErrorStream))
      var e = ""
      while ( {(e = err.readLine) != null}) {
        System.out.println("ERROR:"+e)
      }

      }

    spark.stop()

  }
}

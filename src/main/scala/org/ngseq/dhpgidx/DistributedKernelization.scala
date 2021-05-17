package org.ngseq.dhpgidx

import java.io._
import java.net.URI
import java.text.DecimalFormat

import org.apache.commons.cli.{BasicParser, CommandLine, Option, Options, ParseException}
import org.apache.hadoop.conf.Configuration
import org.apache.hadoop.fs.{FSDataOutputStream, FileSystem, Path}
import org.apache.hadoop.hdfs.DFSClient
import org.apache.hadoop.io.IOUtils
import org.apache.spark.sql.SparkSession

import scala.sys.process._


object DistributedKernelization{

  def main(args: Array[String]) {

    val spark = SparkSession.builder.appName("DistributedIndexing").getOrCreate()

    val options = new Options()
    options.addOption(new Option("lzin", true, "HDFS path for lz input"))
    options.addOption(new Option("in", true, "HDFS path for input"))
    options.addOption(new Option("qlen", true, "Maximum supported query size"))
    options.addOption(new Option("hdfs", true, "HDFS namenode URL"))
    options.addOption(new Option("local", true, "Local path for intermediate files"))
    options.addOption(new Option("threads", true, "Number of threads"))
    options.addOption(new Option("kernel", true, "Kernel type: BLAST, BOWTIE2 or BWA"))
    options.addOption(new Option("chrs", true, "Range of chromosome, or list eg. 1-22 or 1,2,3"))


    val parser = new BasicParser()
    var cmd: CommandLine = null
    try {
      cmd = parser.parse(options, args)
    }catch {
      case exp: ParseException =>
        // oops, something went wrong
        System.err.println("Parsing failed.  Reason: " + exp.getMessage)
    }

    val nf = new DecimalFormat("#00")

    val dataPath = cmd.getOptionValue("in")
    val lzpath = cmd.getOptionValue("lzin")
    val localpath = cmd.getOptionValue("local")
    val maxqlen = cmd.getOptionValue("qlen").toInt
    val hdfsurl = cmd.getOptionValue("hdfs")
    val threads = cmd.getOptionValue("threads").toInt
    val kernel = cmd.getOptionValue("kernel")
    val nchrs = cmd.getOptionValue("chrs")
    var chrs = Seq(0)
    if(nchrs.contains("-")){
      val s = nchrs.split("-").map(_.toInt)
      chrs = (s(0) to s(1)).toSeq
    }else{
      val ls = nchrs.split(",").map(_.toInt)
      chrs = ls.toSeq
    }


    println("Load and preprocess pan-genome")

    val files = spark.sparkContext.parallelize(chrs)

    files.foreach{chr =>

      //var fis: FSDataInputStream = null

      val fs = FileSystem.get(new URI(hdfsurl),new Configuration())
      val ls = FileSystem.getLocal(new Configuration())
      val client = new DFSClient(URI.create(hdfsurl), new Configuration())
      val st = fs.listStatus(new Path(dataPath))

      var los: FSDataOutputStream = null

      try {
        los = ls.create(new Path(localpath+"/chr"+nf.format(chr)+".fa"))
      } catch {
        case e: java.io.IOException =>
          e.printStackTrace()
      }
      val writer = new BufferedWriter(new OutputStreamWriter(los))

      st.foreach{s=>
        if(s.getPath.getName.endsWith(nf.format(chr))){
          val hdfsstream = client.open(dataPath+"/"+s.getPath.getName)
          val hdfsinput = new BufferedReader(new InputStreamReader(hdfsstream))

          var l = hdfsinput.readLine()
          while ( l != null ) {
            writer.write(l)
            writer.newLine()
            l=hdfsinput.readLine()
          }

          /*while(byte==fis.read()!=null){
            los.writeChar(byte)
          }
          fis.close()*/
          hdfsinput.close()
          hdfsstream.close()
        }

      }
      writer.close()
      los.close()

      var los2 = new FileOutputStream(localpath+"/chr"+nf.format(chr)+".lz")

      //val writer2 = new BufferedWriter(new OutputStreamWriter(los2))

      val lzs = fs.listStatus(new Path(lzpath))
      lzs.foreach{s=>
        if(s.getPath.getName.endsWith(nf.format(chr))){

          val hdfsstream = client.open(lzpath+"/"+s.getPath.getName)
          IOUtils.copyBytes(hdfsstream, los2, new Configuration(), false)
        }
          //org.apache.hadoop.fs.FileUtil.copyMerge(fs, s.getPath, ls, new Path(localpath+"/chr"+nf.format(chr)+".lz"), false, new Configuration(), null)
      }
      los2.close()


      //org.apache.hadoop.fs.FileUtil.copyMerge(fs, new Path(dataPath+"/*."+nf.format(chr)), ls, new Path(localpath+"/chr"+nf.format(chr)+".fa"), false, new Configuration(), null)

      //org.apache.hadoop.fs.FileUtil.copyMerge(fs, new Path(lzpath+"/*."+nf.format(chr)), ls, new Path(localpath+"/chr"+nf.format(chr)+".lz"), false, new Configuration(), null)
      //  FileSystem srcFS, Path srcDir, FileSystem dstFS, Path dstFile, boolean deleteSource, Configuration conf, String addString
      //org.apache.hadoop.fs.FileUtil.copyMerge(fs, new Path(lzpath+"/"+bname+".lz")
      //  FileSystem srcFS, Path srcDir, FileSystem dstFS, Path dstFile, boolean deleteSource, Configuration conf, String addString


      val out = new StringBuilder
      val err = new StringBuilder

      val logger = ProcessLogger(
        (o: String) => out.append(o),
        (e: String) => err.append(e))

      //-o "+localpath+"/chr"+chr+".fa
      val chicindex = Process("/opt/dhpgindex/chic_index --threads="+threads+"  --kernel="+kernel+" --kernelize --verbose=2 --lz-parsing-method=RLZ --lz-input-plain-file="+localpath+"/chr"+nf.format(chr)+".lz  "+localpath+"/chr"+nf.format(chr)+".fa "+maxqlen).!
      //val status = chicproc.!(logger)


      }

    spark.stop()

  }
}

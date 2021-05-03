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
import scala.sys.process._
import org.apache.commons.cli.{BasicParser, CommandLine, Option, Options, ParseException}


object DistributedIndexing{

  def main(args: Array[String]) {

    val spark = SparkSession.builder.appName("DistributedIndexing").getOrCreate()

    val options = new Options()
    options.addOption(new Option("lzin", true, "HDFS path for lz input"))
    options.addOption(new Option("in", true, "HDFS path for input"))
    options.addOption(new Option("qlen", true, "Maximum supported query size"))
    options.addOption(new Option("hdfs", true, "HDFS namenode URL"))
    options.addOption(new Option("local", true, "Local path for intermediate files"))
    options.addOption(new Option("threads", true, "Number of threads"))

    val qfile = new Option("qfile", true,"Path to query file in HDFS if distributed alignment is used")
    qfile.setRequired(false)
    options.addOption(qfile)

    val parser = new BasicParser()
    var cmd: CommandLine = null
    try {
      cmd = parser.parse(options, args)
    }catch {
      case exp: ParseException =>
        // oops, something went wrong
        System.err.println("Parsing failed.  Reason: " + exp.getMessage)
    }


    val dataPath = cmd.getOptionValue("in")
    val lzpath = cmd.getOptionValue("lzin")
    val localpath = cmd.getOptionValue("local")
    val maxqlen = cmd.getOptionValue("qlen").toInt
    val hdfsurl = cmd.getOptionValue("hdfs")
    val threads = cmd.getOptionValue("threads").toInt

    println("Load and preprocess pan-genome")

    val files = spark.sparkContext.wholeTextFiles(dataPath)

    files.foreach{file =>

      //var fis: FSDataInputStream = null
      val fs = FileSystem.get(new URI(hdfsurl),new Configuration())
      val fname=(new Path(file._1)).getName()
      fs.copyToLocalFile(new Path(file._1),new Path(localpath+"/"+fname))
      val bname = fname.substring(0,fname.lastIndexOf("."))
      fs.copyToLocalFile(new Path(lzpath+"/"+bname+".lz"),new Path(localpath+"/"+bname+".lz"))

      val out = new StringBuilder
      val err = new StringBuilder

      val logger = ProcessLogger(
        (o: String) => out.append(o),
        (e: String) => err.append(e))

      val chicindex = Process("/opt/dhpgindex/chic_index --threads="+threads+"  --kernel=BLAST --verbose=2 --lz-parsing-method=RELZ --lz-input-plain-file="+localpath+"/"+bname+".lz -o "+localpath+"/"+fname+" "+localpath+"/"+fname+" "+maxqlen).!
      //val status = chicproc.!(logger)

      //if(status!=0) System.out.println(err.toString())

      if(cmd.hasOption("qfile")){
        val hdfsqfile = new Path(cmd.getOptionValue("qfile"))
        val qname=hdfsqfile.getName()
        val localqfile = new Path(localpath+"/"+qname)
        if(!fs.isFile(localqfile)){
          fs.copyToLocalFile(hdfsqfile,localqfile)
        }

        val chicalign= Process("/opt/dhpgindex/chic_align --threads="+threads+"--verbose=2 -o "+localpath+"/"+fname+".blast "+localpath+"/"+fname+" "+localqfile).!

        Process("hdfs dfs -put "+localpath+"/"+fname+".blast blasted/").!

      }

      }

    spark.stop()

  }
}

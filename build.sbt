name := "panquery"

version := "0.1"

scalaVersion := "2.11.0"

libraryDependencies += "org.apache.spark" %% "spark-sql" % "2.3.1"

libraryDependencies += "org.apache.hbase" % "hbase-client" % "2.2.0"
libraryDependencies += "org.apache.hbase" % "hbase-common" % "2.2.0"
libraryDependencies += "org.apache.hbase" % "hbase" % "2.2.0"
libraryDependencies += "org.xerial.larray" %% "larray" % "0.4.0"

resolvers += "Sonatype shapshot repo" at "https://oss.sonatype.org/content/repositories/snapshots/"
resolvers ++= Seq(
  "Apache Repository" at "https://repository.apache.org/content/repositories/releases/"
)
resolvers += Resolver.mavenLocal

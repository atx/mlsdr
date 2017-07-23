organization := "atx.name"

version := "0.0-SNAPSHOT"

name := "mlsdr"

scalaVersion := "2.11.7"

scalacOptions ++= Seq("-deprecation", "-feature", "-unchecked", "-language:reflectiveCalls")

val defaultVersions = Map(
	"chisel3" -> "3.0-SNAPSHOT",
	"chisel-iotesters" -> "1.1-SNAPSHOT"
)

libraryDependencies ++= (Seq("chisel3","chisel-iotesters").map {
	dep: String => "edu.berkeley.cs" %% dep % sys.props.getOrElse(dep + "Version", defaultVersions(dep)) })

resolvers ++= Seq(
	Resolver.sonatypeRepo("snapshots"),
	Resolver.sonatypeRepo("releases")
)

initialCommands in console += "import chisel3._"

logBuffered in Test := false
parallelExecution in Test := false

resolvers +=  Resolver.url("Navetas Repo", url("http://github.lan.ise-oxford.com/Navetas/navetasivyrepo/raw/master/releases"))( Patterns("[organisation]/[module]_[scalaVersion]_[sbtVersion]/[revision]/[artifact]-[revision].[ext]") )

addSbtPlugin("com.navetas" % "navetas-sbt-cpp" % "0.0.34")


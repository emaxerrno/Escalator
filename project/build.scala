import sbt._
import Keys._
import org.seacourt.build._
import org.seacourt.build.NativeBuild._
import org.seacourt.build.NativeDefaultBuild._

object TestBuild extends NativeDefaultBuild( "TestBuild" )
{
    import NativeProject._
    
    lazy val commonLibs = Seq("boost_thread", "boost_system", "boost_unit_test_framework", "pthread")
    
    lazy val escalator = NativeProject( "escalator", file( "escalator" ),
        staticLibrarySettings ++ Seq
        (
            nativeLibraries in Compile      ++=     commonLibs,
            nativeLibraries in Test         ++=     commonLibs
        ) )
    
}

set -e

rm -rf target
./sbt "nativeBuildConfiguration Gcc_LinuxPC_Release" test
#./sbt "native-build-configuration Clang_LinuxPC_Release" test

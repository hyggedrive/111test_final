#!/bin/bash

# Tools to use for bootstrapping.
C_COMPILER_PATH=gcc
CXX_COMPILER_PATH=g++

# Initialize our own variables.
backends="all"
build_for_embedded="0"
build_for_openeuler="0"
buildtype="RelWithDebInfo"
clean="0"
containerize="0"
containerize_needed="0"
container="openEuler"
docker=$(type -p docker)
do_install="0"
enable_acpo="1"
enable_autotuner="1"
enable_bolt="1"
enable_classic_flang="0"
enabled_projects="clang;lld;openmp;clang-tools-extra"
host_arch="$(uname -m)"
install="install"
install_toolchain_only="0"
split_dwarf="on"
unit_test=""
use_ccache="0"
verbose=""

dir="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null && pwd )"
build_dir_name="build"
install_dir_name="install"
build_prefix="$dir/$build_dir_name"
install_prefix="$dir/$install_dir_name"

# Use 8 threads for builds and tests by default. Use more threads if possible,
# but avoid overloading the system by using up to 50% of available cores.
threads=8
nproc=$(type -p nproc)
if [ -x "$nproc" -a -f /proc/loadavg ]; then
  loadavg=$(awk '{printf "%.0f\n", $1}' < /proc/loadavg)
  let threads="($($nproc) - $loadavg) / 2"
  if [ $threads -le 0 ]; then
    threads=1
  fi
fi

# Exit script on first error.
set -e

usage() {
  cat <<EOF
Usage: $0 [options]

Build the compiler under $build_prefix, then install under $install_prefix.

Options:
  -a       Disable BiSheng-Autotuner.
  -A       Disable ACPO.
  -b type  Specify CMake build type (default: $buildtype).
  -c       Use ccache (default: $use_ccache).
  -C       Containerize the build for compatibility.
  -d dir   Specify build directory name (default: "$build_dir_name").
  -D env   Use openEuler/CentOS docker container for containerize build (default: $container).
  -e       Build for embedded cross tool chain.
  -E       Build for openEuler.
  -f       Enable classic flang.
  -h       Display this help message.
  -i       Install the build (default: $do_install).
  -I name  Specify install directory name (default: "$install_dir_name").
  -j N     Allow N jobs at once (default: $threads).
  -o       Enable LLVM_INSTALL_TOOLCHAIN_ONLY=ON.
  -O       Do not build BOLT (binary optimization tool).
  -r       Delete $install_prefix and perform a clean build (default: incremental).
  -s       Strip binaries and minimize file permissions when (re-)installing.
  -t       Enable unit tests for components that support them (make check-all).
  -v       Enable verbose build output (default: quiet).
  -X archs Build only the specified semi-colon-delimited list of backends (default: "$backends").
EOF
}

# Process command-line options. Remember the options for passing to the
# containerized build script.
containerized_opts=()
while getopts :aAb:cCd:D:eEfhiI:j:oOrstvX: optchr; do
  case "$optchr" in
    a)
      enable_autotuner="0"
      containerized_opts+=(-$optchr)
      ;;
    A)
      enable_acpo="0"
      containerized_opts+=(-$optchr)
      ;;
    b)
      buildtype="$OPTARG"
      case "${buildtype,,}" in
        release)
          split_dwarf=off
          ;;
        debug|relwithdebinfo)
          ;;
        *)
          echo "$0: invalid build type '$buildtype'"
          exit 1
          ;;
      esac
      containerized_opts+=(-$optchr "$OPTARG")
      ;;
    c)
      use_ccache="1"
      containerized_opts+=(-$optchr)
      ;;
    C)
      if [ -z "$docker" -o ! -x "$docker" ]; then
        echo "$0: no usable Docker"
        exit 1
      fi
      containerize=1
      ;;
    d)
      build_dir_name="$OPTARG"
      build_prefix="$dir/$build_dir_name"
      containerized_opts+=(-$optchr "$OPTARG")
      ;;
    D)
      container="$OPTARG"
      containerize_needed=1
      case "${container}" in
        openEuler)
          ;;
        CentOS)
          if [ "$(uname -m)" != "aarch64" ]; then
            echo "$0: CentOS container only support AArch64 for now"
            exit 1
          fi
          ;;
        *)
          echo "$0: invalid container env '$container'"
          exit 1
          ;;
      esac
      ;;
    e)
      build_for_embedded="1"
      containerized_opts+=(-$optchr)
      ;;
    E)
      build_for_openeuler="1"
      containerized_opts+=(-$optchr)
      ;;
    f)
      enable_classic_flang="1"
      containerized_opts+=(-$optchr)
      ;;
    h)
      usage
      exit
      ;;
    i)
      do_install="1"
      containerized_opts+=(-$optchr)
      ;;
    I)
      install_dir_name="$OPTARG"
      install_prefix="$dir/$install_dir_name"
      containerized_opts+=(-$optchr "$OPTARG")
      ;;
    j)
      threads="$OPTARG"
      containerized_opts+=(-$optchr "$OPTARG")
      ;;
    o)
      install_toolchain_only=1
      containerized_opts+=(-$optchr)
      ;;
    O)
      enable_bolt="0"
      containerized_opts+=(-$optchr)
      ;;
    r)
      clean=1
      containerized_opts+=(-$optchr)
      ;;
    s)
      install="install/strip"
      containerized_opts+=(-$optchr)
      ;;
    t)
      unit_test=check-all
      containerized_opts+=(-$optchr)
      ;;
    v)
      verbose="VERBOSE=1"
      containerized_opts+=(-$optchr)
      ;;
    X)
      backends="$OPTARG"
      containerized_opts+=(-$optchr "$OPTARG")
      ;;
    :)
      echo "$0: missing argument for option '-$OPTARG'"
      exit 1
      ;;
    ?)
      echo "$0: invalid option '-$OPTARG'"
      exit 1
      ;;
  esac
done

if [ $OPTIND -le $# ]; then
  echo "$0: invalid option '${@:$OPTIND:1}'"
  exit 1
fi

# Make sure that all files under the build directory can be deleted; when some
# LLVM tests are interrupted, they can leave behind inaccessible directories.
build_cleanup() {
  chmod -R u+rwX,go+rX $build_prefix > /dev/null 2>&1
}

# Handle interrupts. When not running a containerized build, we have to enable
# job control (-m), and make sure to delete our own long-running child
# processes. In particular, ninja and python (llvm-lit) refuse to terminate
# when Jenkins aborts the parent process and disconnects.
set -m
handle_abort() {
  local rc=$1 sig=$2
  build_cleanup
  trap - EXIT SIGHUP SIGINT SIGTERM
  local children="$(jobs -p)"
  for cgrp in $children ; do
    kill -$sig -${cgrp} > /dev/null 2>&1
  done
  if [ -n "$sig" ]; then
    kill -$sig 0
  else
    exit $rc
  fi
}

if [ $containerize -eq 0 ]; then
  trap - SIGCHLD
  trap 'handle_abort $?' EXIT
  trap 'handle_abort 129 HUP' SIGHUP
  trap 'handle_abort 130 INT' SIGINT
  trap 'handle_abort 143 TERM' SIGTERM
  if [ $containerize_needed -eq 1 ]; then
    echo "$0: -C is needed for containerize build"
    exit 1
  fi
else
  cmd=$(readlink --canonicalize-existing $0)

  # Generate passwd/group files for the container.
  # lit.py depends on correct results from getpwent.
  homedir=$(realpath $HOME)
  passwd=$(mktemp /tmp/passwd.XXXXXX)
  echo "root:x:0:0::/root:/bin/bash" > $passwd
  echo "user:x:$(id -u):$(id -g)::$homedir:/bin/bash" >> $passwd

  group=$(mktemp /tmp/group.XXXXXX)
  echo "root:x:0:" > $group
  echo "users:x:$(id -g):" >> $group

  # Re-run myself in a openEuler container. The --cap-add option is needed
  # to pacify llvm-exegesis unit tests. Make sure that the container is
  # stopped if the child process is interrupted (e.g. by Jenkins).
  # Note that the container ID file is created with `mktemp -u` because
  # `docker run` refuses to overwrite an existing ID file.
  echo "Re-launching in container."
  containerid=$(mktemp -u /tmp/docker-cid.$$.XXXXXX)
  docker_cleanup() {
    local rc=$1 sig=$2
    docker container stop $(cat $containerid) > /dev/null 2>&1
    rm $passwd $group $containerid
    build_cleanup
    trap - EXIT SIGHUP SIGINT SIGTERM # avoid infinite recursion
    if [ -n "$sig" ]; then
      kill -$sig 0
    else
      exit $rc
    fi
  }
  trap 'docker_cleanup $?' EXIT
  trap 'docker_cleanup 129 HUP' SIGHUP
  trap 'docker_cleanup 130 INT' SIGINT
  trap 'docker_cleanup 143 TERM' SIGTERM

  if [ "$container" == "CentOS" ]; then
    DOCKER_IMAGE="swr.cn-north-4.myhuaweicloud.com/llvm4oe/llvm-build-dep-centos7.6:latest"
  else
    DOCKER_IMAGE="hub.oepkgs.net/openeuler/llvm-build-deps:latest"
  fi
  docker_opts="--rm
    --cap-add=SYS_ADMIN
    --cap-add=SYS_PTRACE
    --security-opt seccomp=unconfined
    --user $(id -u):$(id -g)
    --workdir=$(realpath $PWD)
    --ulimit core=0
    --ulimit stack=-1
    --cidfile $containerid
    -v $homedir:$homedir
    -v $passwd:/etc/passwd
    -v $group:/etc/group
    -e BINUTILS_INCDIR=/usr/local/include
    ${DOCKER_IMAGE}"

  if [ -t 1 ]; then
    $docker run -it $docker_opts ${cmd} ${containerized_opts[@]}
    exit $?
  else
    set -x
    $docker run $docker_opts ${cmd} ${containerized_opts[@]} &
    wait $! || exit $?
    exit 0
  fi
fi

echo "Using $threads threads."

CMAKE_OPTIONS="-DCMAKE_INSTALL_PREFIX=$install_prefix \
               -DCMAKE_BUILD_TYPE=$buildtype \
               -DCMAKE_C_COMPILER=$C_COMPILER_PATH \
               -DCMAKE_CXX_COMPILER=$CXX_COMPILER_PATH \
               -DLLVM_TARGETS_TO_BUILD=$backends "

gold=$(type -p ld.gold)
if [ -z "$gold" -o ! -x "$gold" ]; then
  echo "$0: no usable ld.gold"
  exit 1
fi

# If the invocation does not force a particular binutils installation, check
# that we are using an acceptable version.
if [ -n "$BINUTILS_INCDIR" ]; then
  llvm_binutils_incdir="-DLLVM_BINUTILS_INCDIR=$BINUTILS_INCDIR"
else
  incdir=$(realpath --canonicalize-existing $(dirname $gold)/../include)
  if [ -z "$incdir" -o ! -f "$incdir/plugin-api.h" ]; then
    echo "$0: plugin-api.h not found; required to build LLVMgold.so"
    echo "$0: Try 'yum install binutils-devel', 'apt install binutils-dev', etc"
    exit 1
  fi
  llvm_binutils_incdir="-DLLVM_BINUTILS_INCDIR=$incdir"
fi

if [ $use_ccache == "1" ]; then
  echo "Build using ccache"
  CMAKE_OPTIONS="$CMAKE_OPTIONS \
                -DCMAKE_C_COMPILER_LAUNCHER=ccache \
                -DCMAKE_CXX_COMPILER_LAUNCHER=ccache "
fi

# When set LLVM_INSTALL_TOOLCHAIN_ONLY to On it removes many of the LLVM development
# and testing tools as well as component libraries from the default install target.
if [ $install_toolchain_only == "1" ]; then
  echo "Only install toolchain"
  CMAKE_OPTIONS="$CMAKE_OPTIONS -DLLVM_INSTALL_TOOLCHAIN_ONLY=ON"
fi

# Process the enabling of features.

if [ $enable_autotuner == "1" ]; then
  echo "Enable BiSheng-Autotuner"
  CMAKE_OPTIONS="$CMAKE_OPTIONS -DLLVM_ENABLE_AUTOTUNER=ON"
fi

if [ $enable_acpo == "1" ]; then
  echo "Enable ACPO"
  export CFLAGS="-Wp,-DENABLE_ACPO ${CFLAGS}"
  export CXXFLAGS="-Wp,-DENABLE_ACPO ${CXXFLAGS}"
  # Actually we do not support '-DENABLE_ACPO=ON' cmake option now.
  # CMAKE_OPTIONS="$CMAKE_OPTIONS -DENABLE_ACPO=ON"
fi

if [ $enable_bolt == "1" ]; then
  echo "Enable BOLT"
  # There is an internal error when linking with gold while compiling BOLT.
  unset llvm_use_linker
  enabled_projects+=";bolt"
  EXE_LINKER_FLAGS="-Wl,--compress-debug-sections=zlib" 
else
  llvm_use_linker="-DLLVM_USE_LINKER=gold"
  EXE_LINKER_FLAGS="-Wl,--gdb-index -Wl,--compress-debug-sections=zlib" 
fi

if [ $enable_classic_flang == "1" ]; then
  echo "Enable classic flang"
  CMAKE_OPTIONS="$CMAKE_OPTIONS \
                -DLLVM_ENABLE_CLASSIC_FLANG=on"
fi

# Process the enabling of platforms.

if [ $build_for_embedded == "1" ]; then
  echo "Build for embedded cross tool chain"
  echo "Only enable clang and lld in '-DLLVM_ENABLE_PROJECTS'"
  # Rewrite enabled_projects to enable clang and lld only,
  # drop bolt that may exist.
  enabled_projects="clang;lld"
  CMAKE_OPTIONS="$CMAKE_OPTIONS \
                -DLLVM_BUILD_FOR_EMBEDDED=ON"
fi

if [ $build_for_openeuler == "1" ]; then
  echo "Build for openEuler"
  CMAKE_OPTIONS="$CMAKE_OPTIONS -DBUILD_FOR_OPENEULER=ON"
fi

if [ -n "$verbose" ]; then
  CMAKE_OPTIONS="$CMAKE_OPTIONS -DCMAKE_VERBOSE_MAKEFILE=ON"
  LIT_ARGS="-vv"
else
  LIT_ARGS="-sv"
fi

# Build and install
if [ $clean -eq 1 -a -e "$install_prefix" ]; then
  rm -rf "$install_prefix"
fi
mkdir -p "$install_prefix/bin"

if [ $clean -eq 1 -a -e "$build_prefix" ]; then
  rm -rf "$build_prefix"
fi

mkdir -p "$build_prefix" && cd "$build_prefix"
cmake $CMAKE_OPTIONS \
      -DBUILD_SHARED_LIBS=OFF \
      -DCLANG_DEFAULT_PIE_ON_LINUX=ON \
      -DCLANG_DEFAULT_UNWINDLIB=libgcc \
      -DCLANG_ENABLE_ARCMT=ON \
      -DCLANG_ENABLE_STATIC_ANALYZER=ON \
      -DCLANG_PLUGIN_SUPPORT=ON \
      -DCMAKE_EXE_LINKER_FLAGS_DEBUG=$EXE_LINKER_FLAGS \
      -DCMAKE_EXE_LINKER_FLAGS_RELWITHDEBINFO=$EXE_LINKER_FLAGS \
      -DCMAKE_SKIP_RPATH=ON \
      -DCOMPILER_RT_BUILD_SANITIZERS=on \
      -DENABLE_LINKER_BUILD_ID=ON \
      -DLIBCXX_ENABLE_ABI_LINKER_SCRIPT=ON \
      -DLIBCXX_STATICALLY_LINK_ABI_IN_STATIC_LIBRARY=ON \
      -DLIBOMP_INSTALL_ALIASES=OFF \
      -DLLVM_BUILD_EXAMPLES=OFF \
      -DLLVM_BUILD_RUNTIME=ON \
      -DLLVM_BUILD_TESTS=ON \
      -DLLVM_BUILD_TOOLS=ON \
      -DLLVM_DYLIB_COMPONENTS="all" \
      -DLLVM_ENABLE_EH=ON \
      -DLLVM_ENABLE_FFI=ON \
      -DLLVM_ENABLE_PER_TARGET_RUNTIME_DIR=ON \
      -DLLVM_ENABLE_PROJECTS=$enabled_projects \
      -DLLVM_ENABLE_RTTI=ON \
      -DLLVM_ENABLE_RUNTIMES="compiler-rt;libunwind" \
      -DLLVM_ENABLE_TERMINFO=NO \
      -DLLVM_ENABLE_ZLIB=ON \
      -DLLVM_INCLUDE_BENCHMARKS=OFF \
      -DLLVM_INCLUDE_EXAMPLES=ON \
      -DLLVM_INCLUDE_TESTS=ON \
      -DLLVM_INCLUDE_TOOLS=ON \
      -DLLVM_INCLUDE_UTILS=ON \
      -DLLVM_INSTALL_GTEST=ON \
      -DLLVM_INSTALL_UTILS=ON \
      -DLLVM_LIT_ARGS="$LIT_ARGS -j$threads" \
      -DLLVM_STATIC_LINK_CXX_STDLIB=ON \
      -DLLVM_USE_PERF=ON \
      -DLLVM_USE_SPLIT_DWARF=$split_dwarf \
      $llvm_binutils_incdir \
      $llvm_use_linker \
      ../llvm

make -j$threads $verbose
if [ $do_install == "1" ]; then
  make -j$threads $verbose $install
fi

# build libcxx/libcxxabi with the just-built clang/clang++
c_compiler="$build_prefix/bin/clang"
cxx_compiler="$build_prefix/bin/clang++"
if pushd runtimes > /dev/null 2>&1; then
  if [ ! -f "$build_prefix"/projects/libcxx/CMakeCache.txt ]; then
    mkdir -p "$build_prefix/projects/libcxx" && cd "$build_prefix/projects/libcxx"
    cmake -Wno-dev \
          -DBUILD_SHARED_LIBS=OFF \
          -DCMAKE_BUILD_TYPE=$buildtype \
          -DCMAKE_C_COMPILER="$c_compiler" \
          -DCMAKE_CXX_COMPILER="$cxx_compiler" \
          -DCMAKE_INSTALL_PREFIX="$install_prefix" \
          -DCMAKE_SKIP_RPATH=ON \
          -DLIBCXX_ENABLE_ASSERTIONS=OFF \
          -DLLVM_ENABLE_RUNTIMES="libcxx;libcxxabi" \
          -DLLVM_LIT_ARGS="$LIT_ARGS -j$threads" \
          -DLLVM_USE_SPLIT_DWARF=$split_dwarf \
          $llvm_use_linker \
          ../../../runtimes
  else
    cd "$build_prefix"/projects/libcxx
  fi
  install_libcxx=${install/\/strip/-stripped}
  make -j$threads $verbose \
    ${install_libcxx/install/install-cxx} ${install_libcxx/install/install-cxxabi} ${install_libcxx/install/install-cxxabi-headers}
  if [ -n "$unit_test" ]; then
    make -j$threads $verbose ${unit_test/all/cxx} ${unit_test/all/cxxabi}
  fi
  popd > /dev/null 2>&1
else
  echo "$0: directory not found: runtimes"
  exit 1
fi

if [ -n "$unit_test" ]; then
  make -j$threads $verbose check-all
fi

# When building official deliverables, minimize file permissions under the
# installation directory.
if [ "$install" = "install/strip" ]; then
  find $install_prefix/bin/ -type f -exec strip {} \;
  find $install_prefix -type f -exec chmod a-w {} \;
fi

# In openEuler embedded building system, it need wrap llvm-readelf
# to replace binutils-readelf.
if [ -e "$install_prefix/bin/llvm-readobj" ]; then
  ln -sf llvm-readobj $install_prefix/bin/llvm-readelf
fi

echo "$0: SUCCESS"

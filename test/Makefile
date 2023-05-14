

# Debug or Release or ...
# BUILDTYPE=Debug
BUILDTYPE=Release
BUILDTARGET=common

JOBS=$(shell grep cpu.cores /proc/cpuinfo | sort -u | sed 's/[^0-9]//g')

cmake_configure:
	set -e; mkdir -p build; cd build; cmake -DCMAKE_BUILD_TYPE=${BUILDTYPE} -D BUILD_TARGET=${BUILDTARGET} -G "Unix Makefiles" ../

cmake_debug_configure:
	set -e; mkdir -p build; cd build; cmake -DCMAKE_BUILD_TYPE=Debug -D BUILD_TARGET=${BUILDTARGET} -G "Unix Makefiles" ../

all: cmake_configure
	set -e; cd build; cmake --build . -j ${JOBS} -v

debug-all: cmake_debug_configure
	set -e; cd build; cmake --build . -j ${JOBS} -v

test: debug-all
	set -e; cd build; cmake --build . -j ${JOBS} --target test

clean:
	-rm -fr build

echo:
	echo JOBS = ${JOBS}

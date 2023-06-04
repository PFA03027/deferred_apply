

# Debug or Release or ...
# BUILDTYPE=Debug
BUILDTYPE=Release
BUILDTARGET=common

PROCESSORS=$(shell grep processor /proc/cpuinfo|wc -l)
JOBS=$(shell expr $(PROCESSORS) - $(PROCESSORS) / 4 )

cmake_configure:
	set -e; mkdir -p build; cd build; cmake -DCMAKE_BUILD_TYPE=${BUILDTYPE} -DBUILD_TARGET=${BUILDTARGET} -G "Unix Makefiles" ../

cmake_debug_configure:
	set -e; mkdir -p build; cd build; cmake -DCMAKE_BUILD_TYPE=Debug -D BUILD_TARGET=${BUILDTARGET} -G "Unix Makefiles" ../

cmake_codecoverage_configure:
	set -e; mkdir -p build; cd build; cmake -DCMAKE_BUILD_TYPE=Debug -D BUILD_TARGET=codecoverage -G "Unix Makefiles" ../

all: cmake_configure
	set -e; cd build; cmake --build . -j ${JOBS} -v

debug-all: cmake_debug_configure
	set -e; cd build; cmake --build . -j ${JOBS} -v

test: debug-all
	set -e; cd build; cmake --build . -j ${JOBS} --target test

coverage: cmake_codecoverage_configure
	set -e; \
	cd build; \
	cmake --build . -j ${JOBS} -v; \
	cmake --build . -j ${JOBS} --target test; \
	find . -type f -name "*.gcda" | xargs -I@ gcov -l -b @; \
	lcov --rc lcov_branch_coverage=1 -c -d . -o tmp.info; \
	lcov --rc lcov_branch_coverage=1 -b -c -d . -r tmp.info  '/usr/include/*' -o tmp2.info; \
	lcov --rc lcov_branch_coverage=1 -b -c -d . -r tmp2.info  '*/test/*' -o output.info; \
	genhtml --branch-coverage -o OUTPUT -p . -f output.info

clean:
	-rm -fr build

echo:
	echo JOBS = ${JOBS}

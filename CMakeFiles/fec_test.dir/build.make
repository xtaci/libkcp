# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.7

# Delete rule output on recipe failure.
.DELETE_ON_ERROR:


#=============================================================================
# Special targets provided by cmake.

# Disable implicit rules so canonical targets will work.
.SUFFIXES:


# Remove some rules from gmake that .SUFFIXES does not remove.
SUFFIXES =

.SUFFIXES: .hpux_make_needs_suffix_list


# Suppress display of executed commands.
$(VERBOSE).SILENT:


# A target that is always out of date.
cmake_force:

.PHONY : cmake_force

#=============================================================================
# Set environment variables for the build.

# The shell in which to execute make rules.
SHELL = /bin/sh

# The CMake executable.
CMAKE_COMMAND = /usr/local/Cellar/cmake/3.7.2/bin/cmake

# The command to remove a file.
RM = /usr/local/Cellar/cmake/3.7.2/bin/cmake -E remove -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /Users/yarshure/github/libkcp

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /Users/yarshure/github/libkcp

# Include any dependencies generated for this target.
include CMakeFiles/fec_test.dir/depend.make

# Include the progress variables for this target.
include CMakeFiles/fec_test.dir/progress.make

# Include the compile flags for this target's objects.
include CMakeFiles/fec_test.dir/flags.make

CMakeFiles/fec_test.dir/ikcp.c.o: CMakeFiles/fec_test.dir/flags.make
CMakeFiles/fec_test.dir/ikcp.c.o: ikcp.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/Users/yarshure/github/libkcp/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building C object CMakeFiles/fec_test.dir/ikcp.c.o"
	/Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/cc  $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/fec_test.dir/ikcp.c.o   -c /Users/yarshure/github/libkcp/ikcp.c

CMakeFiles/fec_test.dir/ikcp.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/fec_test.dir/ikcp.c.i"
	/Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/cc  $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /Users/yarshure/github/libkcp/ikcp.c > CMakeFiles/fec_test.dir/ikcp.c.i

CMakeFiles/fec_test.dir/ikcp.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/fec_test.dir/ikcp.c.s"
	/Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/cc  $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /Users/yarshure/github/libkcp/ikcp.c -o CMakeFiles/fec_test.dir/ikcp.c.s

CMakeFiles/fec_test.dir/ikcp.c.o.requires:

.PHONY : CMakeFiles/fec_test.dir/ikcp.c.o.requires

CMakeFiles/fec_test.dir/ikcp.c.o.provides: CMakeFiles/fec_test.dir/ikcp.c.o.requires
	$(MAKE) -f CMakeFiles/fec_test.dir/build.make CMakeFiles/fec_test.dir/ikcp.c.o.provides.build
.PHONY : CMakeFiles/fec_test.dir/ikcp.c.o.provides

CMakeFiles/fec_test.dir/ikcp.c.o.provides.build: CMakeFiles/fec_test.dir/ikcp.c.o


CMakeFiles/fec_test.dir/sess.cpp.o: CMakeFiles/fec_test.dir/flags.make
CMakeFiles/fec_test.dir/sess.cpp.o: sess.cpp
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/Users/yarshure/github/libkcp/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Building CXX object CMakeFiles/fec_test.dir/sess.cpp.o"
	/Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/c++   $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/fec_test.dir/sess.cpp.o -c /Users/yarshure/github/libkcp/sess.cpp

CMakeFiles/fec_test.dir/sess.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/fec_test.dir/sess.cpp.i"
	/Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/c++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /Users/yarshure/github/libkcp/sess.cpp > CMakeFiles/fec_test.dir/sess.cpp.i

CMakeFiles/fec_test.dir/sess.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/fec_test.dir/sess.cpp.s"
	/Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/c++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /Users/yarshure/github/libkcp/sess.cpp -o CMakeFiles/fec_test.dir/sess.cpp.s

CMakeFiles/fec_test.dir/sess.cpp.o.requires:

.PHONY : CMakeFiles/fec_test.dir/sess.cpp.o.requires

CMakeFiles/fec_test.dir/sess.cpp.o.provides: CMakeFiles/fec_test.dir/sess.cpp.o.requires
	$(MAKE) -f CMakeFiles/fec_test.dir/build.make CMakeFiles/fec_test.dir/sess.cpp.o.provides.build
.PHONY : CMakeFiles/fec_test.dir/sess.cpp.o.provides

CMakeFiles/fec_test.dir/sess.cpp.o.provides.build: CMakeFiles/fec_test.dir/sess.cpp.o


CMakeFiles/fec_test.dir/galois.cpp.o: CMakeFiles/fec_test.dir/flags.make
CMakeFiles/fec_test.dir/galois.cpp.o: galois.cpp
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/Users/yarshure/github/libkcp/CMakeFiles --progress-num=$(CMAKE_PROGRESS_3) "Building CXX object CMakeFiles/fec_test.dir/galois.cpp.o"
	/Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/c++   $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/fec_test.dir/galois.cpp.o -c /Users/yarshure/github/libkcp/galois.cpp

CMakeFiles/fec_test.dir/galois.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/fec_test.dir/galois.cpp.i"
	/Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/c++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /Users/yarshure/github/libkcp/galois.cpp > CMakeFiles/fec_test.dir/galois.cpp.i

CMakeFiles/fec_test.dir/galois.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/fec_test.dir/galois.cpp.s"
	/Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/c++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /Users/yarshure/github/libkcp/galois.cpp -o CMakeFiles/fec_test.dir/galois.cpp.s

CMakeFiles/fec_test.dir/galois.cpp.o.requires:

.PHONY : CMakeFiles/fec_test.dir/galois.cpp.o.requires

CMakeFiles/fec_test.dir/galois.cpp.o.provides: CMakeFiles/fec_test.dir/galois.cpp.o.requires
	$(MAKE) -f CMakeFiles/fec_test.dir/build.make CMakeFiles/fec_test.dir/galois.cpp.o.provides.build
.PHONY : CMakeFiles/fec_test.dir/galois.cpp.o.provides

CMakeFiles/fec_test.dir/galois.cpp.o.provides.build: CMakeFiles/fec_test.dir/galois.cpp.o


CMakeFiles/fec_test.dir/galois_noasm.cpp.o: CMakeFiles/fec_test.dir/flags.make
CMakeFiles/fec_test.dir/galois_noasm.cpp.o: galois_noasm.cpp
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/Users/yarshure/github/libkcp/CMakeFiles --progress-num=$(CMAKE_PROGRESS_4) "Building CXX object CMakeFiles/fec_test.dir/galois_noasm.cpp.o"
	/Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/c++   $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/fec_test.dir/galois_noasm.cpp.o -c /Users/yarshure/github/libkcp/galois_noasm.cpp

CMakeFiles/fec_test.dir/galois_noasm.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/fec_test.dir/galois_noasm.cpp.i"
	/Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/c++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /Users/yarshure/github/libkcp/galois_noasm.cpp > CMakeFiles/fec_test.dir/galois_noasm.cpp.i

CMakeFiles/fec_test.dir/galois_noasm.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/fec_test.dir/galois_noasm.cpp.s"
	/Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/c++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /Users/yarshure/github/libkcp/galois_noasm.cpp -o CMakeFiles/fec_test.dir/galois_noasm.cpp.s

CMakeFiles/fec_test.dir/galois_noasm.cpp.o.requires:

.PHONY : CMakeFiles/fec_test.dir/galois_noasm.cpp.o.requires

CMakeFiles/fec_test.dir/galois_noasm.cpp.o.provides: CMakeFiles/fec_test.dir/galois_noasm.cpp.o.requires
	$(MAKE) -f CMakeFiles/fec_test.dir/build.make CMakeFiles/fec_test.dir/galois_noasm.cpp.o.provides.build
.PHONY : CMakeFiles/fec_test.dir/galois_noasm.cpp.o.provides

CMakeFiles/fec_test.dir/galois_noasm.cpp.o.provides.build: CMakeFiles/fec_test.dir/galois_noasm.cpp.o


CMakeFiles/fec_test.dir/matrix.cpp.o: CMakeFiles/fec_test.dir/flags.make
CMakeFiles/fec_test.dir/matrix.cpp.o: matrix.cpp
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/Users/yarshure/github/libkcp/CMakeFiles --progress-num=$(CMAKE_PROGRESS_5) "Building CXX object CMakeFiles/fec_test.dir/matrix.cpp.o"
	/Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/c++   $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/fec_test.dir/matrix.cpp.o -c /Users/yarshure/github/libkcp/matrix.cpp

CMakeFiles/fec_test.dir/matrix.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/fec_test.dir/matrix.cpp.i"
	/Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/c++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /Users/yarshure/github/libkcp/matrix.cpp > CMakeFiles/fec_test.dir/matrix.cpp.i

CMakeFiles/fec_test.dir/matrix.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/fec_test.dir/matrix.cpp.s"
	/Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/c++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /Users/yarshure/github/libkcp/matrix.cpp -o CMakeFiles/fec_test.dir/matrix.cpp.s

CMakeFiles/fec_test.dir/matrix.cpp.o.requires:

.PHONY : CMakeFiles/fec_test.dir/matrix.cpp.o.requires

CMakeFiles/fec_test.dir/matrix.cpp.o.provides: CMakeFiles/fec_test.dir/matrix.cpp.o.requires
	$(MAKE) -f CMakeFiles/fec_test.dir/build.make CMakeFiles/fec_test.dir/matrix.cpp.o.provides.build
.PHONY : CMakeFiles/fec_test.dir/matrix.cpp.o.provides

CMakeFiles/fec_test.dir/matrix.cpp.o.provides.build: CMakeFiles/fec_test.dir/matrix.cpp.o


CMakeFiles/fec_test.dir/inversion_tree.cpp.o: CMakeFiles/fec_test.dir/flags.make
CMakeFiles/fec_test.dir/inversion_tree.cpp.o: inversion_tree.cpp
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/Users/yarshure/github/libkcp/CMakeFiles --progress-num=$(CMAKE_PROGRESS_6) "Building CXX object CMakeFiles/fec_test.dir/inversion_tree.cpp.o"
	/Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/c++   $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/fec_test.dir/inversion_tree.cpp.o -c /Users/yarshure/github/libkcp/inversion_tree.cpp

CMakeFiles/fec_test.dir/inversion_tree.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/fec_test.dir/inversion_tree.cpp.i"
	/Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/c++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /Users/yarshure/github/libkcp/inversion_tree.cpp > CMakeFiles/fec_test.dir/inversion_tree.cpp.i

CMakeFiles/fec_test.dir/inversion_tree.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/fec_test.dir/inversion_tree.cpp.s"
	/Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/c++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /Users/yarshure/github/libkcp/inversion_tree.cpp -o CMakeFiles/fec_test.dir/inversion_tree.cpp.s

CMakeFiles/fec_test.dir/inversion_tree.cpp.o.requires:

.PHONY : CMakeFiles/fec_test.dir/inversion_tree.cpp.o.requires

CMakeFiles/fec_test.dir/inversion_tree.cpp.o.provides: CMakeFiles/fec_test.dir/inversion_tree.cpp.o.requires
	$(MAKE) -f CMakeFiles/fec_test.dir/build.make CMakeFiles/fec_test.dir/inversion_tree.cpp.o.provides.build
.PHONY : CMakeFiles/fec_test.dir/inversion_tree.cpp.o.provides

CMakeFiles/fec_test.dir/inversion_tree.cpp.o.provides.build: CMakeFiles/fec_test.dir/inversion_tree.cpp.o


CMakeFiles/fec_test.dir/reedsolomon.cpp.o: CMakeFiles/fec_test.dir/flags.make
CMakeFiles/fec_test.dir/reedsolomon.cpp.o: reedsolomon.cpp
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/Users/yarshure/github/libkcp/CMakeFiles --progress-num=$(CMAKE_PROGRESS_7) "Building CXX object CMakeFiles/fec_test.dir/reedsolomon.cpp.o"
	/Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/c++   $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/fec_test.dir/reedsolomon.cpp.o -c /Users/yarshure/github/libkcp/reedsolomon.cpp

CMakeFiles/fec_test.dir/reedsolomon.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/fec_test.dir/reedsolomon.cpp.i"
	/Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/c++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /Users/yarshure/github/libkcp/reedsolomon.cpp > CMakeFiles/fec_test.dir/reedsolomon.cpp.i

CMakeFiles/fec_test.dir/reedsolomon.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/fec_test.dir/reedsolomon.cpp.s"
	/Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/c++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /Users/yarshure/github/libkcp/reedsolomon.cpp -o CMakeFiles/fec_test.dir/reedsolomon.cpp.s

CMakeFiles/fec_test.dir/reedsolomon.cpp.o.requires:

.PHONY : CMakeFiles/fec_test.dir/reedsolomon.cpp.o.requires

CMakeFiles/fec_test.dir/reedsolomon.cpp.o.provides: CMakeFiles/fec_test.dir/reedsolomon.cpp.o.requires
	$(MAKE) -f CMakeFiles/fec_test.dir/build.make CMakeFiles/fec_test.dir/reedsolomon.cpp.o.provides.build
.PHONY : CMakeFiles/fec_test.dir/reedsolomon.cpp.o.provides

CMakeFiles/fec_test.dir/reedsolomon.cpp.o.provides.build: CMakeFiles/fec_test.dir/reedsolomon.cpp.o


CMakeFiles/fec_test.dir/fec.cpp.o: CMakeFiles/fec_test.dir/flags.make
CMakeFiles/fec_test.dir/fec.cpp.o: fec.cpp
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/Users/yarshure/github/libkcp/CMakeFiles --progress-num=$(CMAKE_PROGRESS_8) "Building CXX object CMakeFiles/fec_test.dir/fec.cpp.o"
	/Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/c++   $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/fec_test.dir/fec.cpp.o -c /Users/yarshure/github/libkcp/fec.cpp

CMakeFiles/fec_test.dir/fec.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/fec_test.dir/fec.cpp.i"
	/Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/c++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /Users/yarshure/github/libkcp/fec.cpp > CMakeFiles/fec_test.dir/fec.cpp.i

CMakeFiles/fec_test.dir/fec.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/fec_test.dir/fec.cpp.s"
	/Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/c++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /Users/yarshure/github/libkcp/fec.cpp -o CMakeFiles/fec_test.dir/fec.cpp.s

CMakeFiles/fec_test.dir/fec.cpp.o.requires:

.PHONY : CMakeFiles/fec_test.dir/fec.cpp.o.requires

CMakeFiles/fec_test.dir/fec.cpp.o.provides: CMakeFiles/fec_test.dir/fec.cpp.o.requires
	$(MAKE) -f CMakeFiles/fec_test.dir/build.make CMakeFiles/fec_test.dir/fec.cpp.o.provides.build
.PHONY : CMakeFiles/fec_test.dir/fec.cpp.o.provides

CMakeFiles/fec_test.dir/fec.cpp.o.provides.build: CMakeFiles/fec_test.dir/fec.cpp.o


CMakeFiles/fec_test.dir/galois_table.c.o: CMakeFiles/fec_test.dir/flags.make
CMakeFiles/fec_test.dir/galois_table.c.o: galois_table.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/Users/yarshure/github/libkcp/CMakeFiles --progress-num=$(CMAKE_PROGRESS_9) "Building C object CMakeFiles/fec_test.dir/galois_table.c.o"
	/Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/cc  $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/fec_test.dir/galois_table.c.o   -c /Users/yarshure/github/libkcp/galois_table.c

CMakeFiles/fec_test.dir/galois_table.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/fec_test.dir/galois_table.c.i"
	/Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/cc  $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /Users/yarshure/github/libkcp/galois_table.c > CMakeFiles/fec_test.dir/galois_table.c.i

CMakeFiles/fec_test.dir/galois_table.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/fec_test.dir/galois_table.c.s"
	/Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/cc  $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /Users/yarshure/github/libkcp/galois_table.c -o CMakeFiles/fec_test.dir/galois_table.c.s

CMakeFiles/fec_test.dir/galois_table.c.o.requires:

.PHONY : CMakeFiles/fec_test.dir/galois_table.c.o.requires

CMakeFiles/fec_test.dir/galois_table.c.o.provides: CMakeFiles/fec_test.dir/galois_table.c.o.requires
	$(MAKE) -f CMakeFiles/fec_test.dir/build.make CMakeFiles/fec_test.dir/galois_table.c.o.provides.build
.PHONY : CMakeFiles/fec_test.dir/galois_table.c.o.provides

CMakeFiles/fec_test.dir/galois_table.c.o.provides.build: CMakeFiles/fec_test.dir/galois_table.c.o


CMakeFiles/fec_test.dir/fec_test.cpp.o: CMakeFiles/fec_test.dir/flags.make
CMakeFiles/fec_test.dir/fec_test.cpp.o: fec_test.cpp
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/Users/yarshure/github/libkcp/CMakeFiles --progress-num=$(CMAKE_PROGRESS_10) "Building CXX object CMakeFiles/fec_test.dir/fec_test.cpp.o"
	/Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/c++   $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/fec_test.dir/fec_test.cpp.o -c /Users/yarshure/github/libkcp/fec_test.cpp

CMakeFiles/fec_test.dir/fec_test.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/fec_test.dir/fec_test.cpp.i"
	/Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/c++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /Users/yarshure/github/libkcp/fec_test.cpp > CMakeFiles/fec_test.dir/fec_test.cpp.i

CMakeFiles/fec_test.dir/fec_test.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/fec_test.dir/fec_test.cpp.s"
	/Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/c++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /Users/yarshure/github/libkcp/fec_test.cpp -o CMakeFiles/fec_test.dir/fec_test.cpp.s

CMakeFiles/fec_test.dir/fec_test.cpp.o.requires:

.PHONY : CMakeFiles/fec_test.dir/fec_test.cpp.o.requires

CMakeFiles/fec_test.dir/fec_test.cpp.o.provides: CMakeFiles/fec_test.dir/fec_test.cpp.o.requires
	$(MAKE) -f CMakeFiles/fec_test.dir/build.make CMakeFiles/fec_test.dir/fec_test.cpp.o.provides.build
.PHONY : CMakeFiles/fec_test.dir/fec_test.cpp.o.provides

CMakeFiles/fec_test.dir/fec_test.cpp.o.provides.build: CMakeFiles/fec_test.dir/fec_test.cpp.o


# Object files for target fec_test
fec_test_OBJECTS = \
"CMakeFiles/fec_test.dir/ikcp.c.o" \
"CMakeFiles/fec_test.dir/sess.cpp.o" \
"CMakeFiles/fec_test.dir/galois.cpp.o" \
"CMakeFiles/fec_test.dir/galois_noasm.cpp.o" \
"CMakeFiles/fec_test.dir/matrix.cpp.o" \
"CMakeFiles/fec_test.dir/inversion_tree.cpp.o" \
"CMakeFiles/fec_test.dir/reedsolomon.cpp.o" \
"CMakeFiles/fec_test.dir/fec.cpp.o" \
"CMakeFiles/fec_test.dir/galois_table.c.o" \
"CMakeFiles/fec_test.dir/fec_test.cpp.o"

# External object files for target fec_test
fec_test_EXTERNAL_OBJECTS =

fec_test: CMakeFiles/fec_test.dir/ikcp.c.o
fec_test: CMakeFiles/fec_test.dir/sess.cpp.o
fec_test: CMakeFiles/fec_test.dir/galois.cpp.o
fec_test: CMakeFiles/fec_test.dir/galois_noasm.cpp.o
fec_test: CMakeFiles/fec_test.dir/matrix.cpp.o
fec_test: CMakeFiles/fec_test.dir/inversion_tree.cpp.o
fec_test: CMakeFiles/fec_test.dir/reedsolomon.cpp.o
fec_test: CMakeFiles/fec_test.dir/fec.cpp.o
fec_test: CMakeFiles/fec_test.dir/galois_table.c.o
fec_test: CMakeFiles/fec_test.dir/fec_test.cpp.o
fec_test: CMakeFiles/fec_test.dir/build.make
fec_test: CMakeFiles/fec_test.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/Users/yarshure/github/libkcp/CMakeFiles --progress-num=$(CMAKE_PROGRESS_11) "Linking CXX executable fec_test"
	$(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/fec_test.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
CMakeFiles/fec_test.dir/build: fec_test

.PHONY : CMakeFiles/fec_test.dir/build

CMakeFiles/fec_test.dir/requires: CMakeFiles/fec_test.dir/ikcp.c.o.requires
CMakeFiles/fec_test.dir/requires: CMakeFiles/fec_test.dir/sess.cpp.o.requires
CMakeFiles/fec_test.dir/requires: CMakeFiles/fec_test.dir/galois.cpp.o.requires
CMakeFiles/fec_test.dir/requires: CMakeFiles/fec_test.dir/galois_noasm.cpp.o.requires
CMakeFiles/fec_test.dir/requires: CMakeFiles/fec_test.dir/matrix.cpp.o.requires
CMakeFiles/fec_test.dir/requires: CMakeFiles/fec_test.dir/inversion_tree.cpp.o.requires
CMakeFiles/fec_test.dir/requires: CMakeFiles/fec_test.dir/reedsolomon.cpp.o.requires
CMakeFiles/fec_test.dir/requires: CMakeFiles/fec_test.dir/fec.cpp.o.requires
CMakeFiles/fec_test.dir/requires: CMakeFiles/fec_test.dir/galois_table.c.o.requires
CMakeFiles/fec_test.dir/requires: CMakeFiles/fec_test.dir/fec_test.cpp.o.requires

.PHONY : CMakeFiles/fec_test.dir/requires

CMakeFiles/fec_test.dir/clean:
	$(CMAKE_COMMAND) -P CMakeFiles/fec_test.dir/cmake_clean.cmake
.PHONY : CMakeFiles/fec_test.dir/clean

CMakeFiles/fec_test.dir/depend:
	cd /Users/yarshure/github/libkcp && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /Users/yarshure/github/libkcp /Users/yarshure/github/libkcp /Users/yarshure/github/libkcp /Users/yarshure/github/libkcp /Users/yarshure/github/libkcp/CMakeFiles/fec_test.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : CMakeFiles/fec_test.dir/depend


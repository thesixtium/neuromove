# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.25

# Delete rule output on recipe failure.
.DELETE_ON_ERROR:

#=============================================================================
# Special targets provided by cmake.

# Disable implicit rules so canonical targets will work.
.SUFFIXES:

# Disable VCS-based implicit rules.
% : %,v

# Disable VCS-based implicit rules.
% : RCS/%

# Disable VCS-based implicit rules.
% : RCS/%,v

# Disable VCS-based implicit rules.
% : SCCS/s.%

# Disable VCS-based implicit rules.
% : s.%

.SUFFIXES: .hpux_make_needs_suffix_list

# Command-line flag to silence nested $(MAKE).
$(VERBOSE)MAKESILENT = -s

#Suppress display of executed commands.
$(VERBOSE).SILENT:

# A target that is always out of date.
cmake_force:
.PHONY : cmake_force

#=============================================================================
# Set environment variables for the build.

# The shell in which to execute make rules.
SHELL = /bin/sh

# The CMake executable.
CMAKE_COMMAND = /usr/bin/cmake

# The command to remove a file.
RM = /usr/bin/cmake -E rm -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /home/pi/Downloads/unilidar_sdk-main/unitree_lidar_sdk

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /home/pi/Downloads/unilidar_sdk-main/unitree_lidar_sdk/build

# Include any dependencies generated for this target.
include CMakeFiles/example_lidar.dir/depend.make
# Include any dependencies generated by the compiler for this target.
include CMakeFiles/example_lidar.dir/compiler_depend.make

# Include the progress variables for this target.
include CMakeFiles/example_lidar.dir/progress.make

# Include the compile flags for this target's objects.
include CMakeFiles/example_lidar.dir/flags.make

CMakeFiles/example_lidar.dir/examples/example_lidar.cpp.o: CMakeFiles/example_lidar.dir/flags.make
CMakeFiles/example_lidar.dir/examples/example_lidar.cpp.o: /home/pi/Downloads/unilidar_sdk-main/unitree_lidar_sdk/examples/example_lidar.cpp
CMakeFiles/example_lidar.dir/examples/example_lidar.cpp.o: CMakeFiles/example_lidar.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/pi/Downloads/unilidar_sdk-main/unitree_lidar_sdk/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building CXX object CMakeFiles/example_lidar.dir/examples/example_lidar.cpp.o"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -MD -MT CMakeFiles/example_lidar.dir/examples/example_lidar.cpp.o -MF CMakeFiles/example_lidar.dir/examples/example_lidar.cpp.o.d -o CMakeFiles/example_lidar.dir/examples/example_lidar.cpp.o -c /home/pi/Downloads/unilidar_sdk-main/unitree_lidar_sdk/examples/example_lidar.cpp

CMakeFiles/example_lidar.dir/examples/example_lidar.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/example_lidar.dir/examples/example_lidar.cpp.i"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/pi/Downloads/unilidar_sdk-main/unitree_lidar_sdk/examples/example_lidar.cpp > CMakeFiles/example_lidar.dir/examples/example_lidar.cpp.i

CMakeFiles/example_lidar.dir/examples/example_lidar.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/example_lidar.dir/examples/example_lidar.cpp.s"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/pi/Downloads/unilidar_sdk-main/unitree_lidar_sdk/examples/example_lidar.cpp -o CMakeFiles/example_lidar.dir/examples/example_lidar.cpp.s

# Object files for target example_lidar
example_lidar_OBJECTS = \
"CMakeFiles/example_lidar.dir/examples/example_lidar.cpp.o"

# External object files for target example_lidar
example_lidar_EXTERNAL_OBJECTS =

/home/pi/Downloads/unilidar_sdk-main/unitree_lidar_sdk/bin/example_lidar: CMakeFiles/example_lidar.dir/examples/example_lidar.cpp.o
/home/pi/Downloads/unilidar_sdk-main/unitree_lidar_sdk/bin/example_lidar: CMakeFiles/example_lidar.dir/build.make
/home/pi/Downloads/unilidar_sdk-main/unitree_lidar_sdk/bin/example_lidar: CMakeFiles/example_lidar.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/home/pi/Downloads/unilidar_sdk-main/unitree_lidar_sdk/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Linking CXX executable /home/pi/Downloads/unilidar_sdk-main/unitree_lidar_sdk/bin/example_lidar"
	$(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/example_lidar.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
CMakeFiles/example_lidar.dir/build: /home/pi/Downloads/unilidar_sdk-main/unitree_lidar_sdk/bin/example_lidar
.PHONY : CMakeFiles/example_lidar.dir/build

CMakeFiles/example_lidar.dir/clean:
	$(CMAKE_COMMAND) -P CMakeFiles/example_lidar.dir/cmake_clean.cmake
.PHONY : CMakeFiles/example_lidar.dir/clean

CMakeFiles/example_lidar.dir/depend:
	cd /home/pi/Downloads/unilidar_sdk-main/unitree_lidar_sdk/build && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/pi/Downloads/unilidar_sdk-main/unitree_lidar_sdk /home/pi/Downloads/unilidar_sdk-main/unitree_lidar_sdk /home/pi/Downloads/unilidar_sdk-main/unitree_lidar_sdk/build /home/pi/Downloads/unilidar_sdk-main/unitree_lidar_sdk/build /home/pi/Downloads/unilidar_sdk-main/unitree_lidar_sdk/build/CMakeFiles/example_lidar.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : CMakeFiles/example_lidar.dir/depend

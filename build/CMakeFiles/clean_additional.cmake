# Additional clean files
cmake_minimum_required(VERSION 3.16)

if("${CONFIG}" STREQUAL "" OR "${CONFIG}" STREQUAL "")
  file(REMOVE_RECURSE
  "CMakeFiles/book-creator-native_autogen.dir/AutogenUsed.txt"
  "CMakeFiles/book-creator-native_autogen.dir/ParseCache.txt"
  "book-creator-native_autogen"
  )
endif()

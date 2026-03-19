# Additional clean files
cmake_minimum_required(VERSION 3.16)

if("${CONFIG}" STREQUAL "" OR "${CONFIG}" STREQUAL "")
  file(REMOVE_RECURSE
  "CMakeFiles/PDFTool_autogen.dir/AutogenUsed.txt"
  "CMakeFiles/PDFTool_autogen.dir/ParseCache.txt"
  "PDFTool_autogen"
  )
endif()

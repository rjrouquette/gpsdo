# the FindSubversion.cmake module is part of the standard distribution
FIND_PACKAGE(Git)
execute_process(COMMAND git log -n 1 --pretty=format:%H OUTPUT_VARIABLE GIT_VERSION)
MESSAGE("Current Git revision is ${GIT_VERSION}")
string(SUBSTRING "${GIT_VERSION}" 0 8 FW_VERSION)
# write a file with the VERSION_GIT define
file(WRITE gitversion.h.txt "#define VERSION_GIT (\"${GIT_VERSION}\")\n#define VERSION_FW (0x${FW_VERSION})\n")
# copy the file to the final header only if the version changes
# reduces needless rebuilds
execute_process(COMMAND ${CMAKE_COMMAND} -E copy_if_different gitversion.h.txt gitversion.h)

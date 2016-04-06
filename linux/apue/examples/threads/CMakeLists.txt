AUX_SOURCE_DIRECTORY(. DIR_SRCS)
add_subdirectory(condvar_ibm_example)
add_executable(threads ${DIR_SRCS})

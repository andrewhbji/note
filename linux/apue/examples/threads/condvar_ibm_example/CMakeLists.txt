AUX_SOURCE_DIRECTORY(. COND_DIR_SRCS)
add_executable(condvar ${COND_DIR_SRCS})
target_link_libraries(condvar pthread)
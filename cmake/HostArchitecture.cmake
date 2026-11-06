if(NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
    message(FATAL_ERROR "Amiberry requires a 64-bit target ABI (8-byte pointers). 32-bit hosts are not supported.")
endif()

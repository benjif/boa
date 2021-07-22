FIND_PROGRAM(NINJA "ninja")

if (NINJA)
    set(CMAKE_GENERATOR "Ninja" CACHE INTERNAL "" FORCE)
else()
    set(CMAKE_GENERATOR "Unix Makefiles" CACHE INTERNAL "" FORCE)
endif()

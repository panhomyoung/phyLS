# Add alice and mockturtle sub directories
add_subdirectory(alice)
add_subdirectory(mockturtle)

set(ABC_USE_NAMESPACE "pabc")

add_subdirectory(abc EXCLUDE_FROM_ALL)
target_compile_definitions(libabc-pic PUBLIC "LIN64")
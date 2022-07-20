
include_directories()

add_library(s7 STATIC s7.c s7.h utils.c)

set_target_properties (s7
   PROPERTIES
      COMPILE_FLAGS "-g -Wall -Wextra"
   )

add_executable(s7scheme s7.c s7.h utils.c repl.c)
set_target_properties(s7scheme PROPERTIES OUTPUT_NAME "s7")
set_target_properties (s7scheme
   PROPERTIES
      COMPILE_FLAGS "-g -Wall -Wextra"
   )

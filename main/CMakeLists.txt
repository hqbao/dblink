set(SOURCES 
  main.c
  server.c)

idf_component_register(
  SRCS ${SOURCES} 
  INCLUDE_DIRS .
  REQUIRES
)
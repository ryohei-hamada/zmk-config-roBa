if(CONFIG_ROBA_RUNTIME_MACROS)
  target_sources(app PRIVATE ${CMAKE_CURRENT_LIST_DIR}/core.c ${CMAKE_CURRENT_LIST_DIR}/module.c)
  target_include_directories(app PRIVATE ${CMAKE_CURRENT_LIST_DIR})
endif()

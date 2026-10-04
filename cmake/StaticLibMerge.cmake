function(_winux_collect_static_libraries target_name scope)
  set(_property_prefix "WINUX_STATIC_MERGE_${scope}")

  if(NOT TARGET "${target_name}")
    get_property(_links GLOBAL PROPERTY "${_property_prefix}_LINKS")
    list(APPEND _links "${target_name}")
    set_property(GLOBAL PROPERTY "${_property_prefix}_LINKS" "${_links}")
    return()
  endif()

  get_target_property(_aliased_target "${target_name}" ALIASED_TARGET)
  if(_aliased_target)
    set(target_name "${_aliased_target}")
  endif()

  get_property(_visited GLOBAL PROPERTY "${_property_prefix}_VISITED")
  if(target_name IN_LIST _visited)
    return()
  endif()
  list(APPEND _visited "${target_name}")
  set_property(GLOBAL PROPERTY "${_property_prefix}_VISITED" "${_visited}")

  get_target_property(_target_type "${target_name}" TYPE)
  if(_target_type STREQUAL "STATIC_LIBRARY")
    get_property(_libraries GLOBAL PROPERTY "${_property_prefix}_LIBRARIES")
    list(APPEND _libraries "${target_name}")
    set_property(GLOBAL PROPERTY "${_property_prefix}_LIBRARIES" "${_libraries}")

    get_target_property(_target_includes "${target_name}" INTERFACE_INCLUDE_DIRECTORIES)
    if(_target_includes)
      get_property(_includes GLOBAL PROPERTY "${_property_prefix}_INCLUDES")
      list(APPEND _includes ${_target_includes})
      set_property(GLOBAL PROPERTY "${_property_prefix}_INCLUDES" "${_includes}")
    endif()

    get_target_property(_target_definitions "${target_name}" INTERFACE_COMPILE_DEFINITIONS)
    if(_target_definitions)
      get_property(_definitions GLOBAL PROPERTY "${_property_prefix}_DEFINITIONS")
      list(APPEND _definitions ${_target_definitions})
      set_property(GLOBAL PROPERTY "${_property_prefix}_DEFINITIONS" "${_definitions}")
    endif()
  endif()

  get_target_property(_target_links "${target_name}" INTERFACE_LINK_LIBRARIES)
  if(_target_links)
    foreach(_dependency IN LISTS _target_links)
      if(_dependency MATCHES "^\\$<LINK_ONLY:([^>]+)>$")
        _winux_collect_static_libraries("${CMAKE_MATCH_1}" "${scope}")
      elseif(_dependency MATCHES "^\\$<")
        get_property(_links GLOBAL PROPERTY "${_property_prefix}_LINKS")
        list(APPEND _links "${_dependency}")
        set_property(GLOBAL PROPERTY "${_property_prefix}_LINKS" "${_links}")
      else()
        _winux_collect_static_libraries("${_dependency}" "${scope}")
      endif()
    endforeach()
  endif()
endfunction()

function(winux_merge_static_libraries target_name)
  if(NOT TARGET "${target_name}")
    message(FATAL_ERROR "Cannot merge archives: target '${target_name}' does not exist.")
  endif()

  string(MAKE_C_IDENTIFIER "${target_name}" _scope)
  set(_property_prefix "WINUX_STATIC_MERGE_${_scope}")
  set_property(GLOBAL PROPERTY "${_property_prefix}_LIBRARIES" "")
  set_property(GLOBAL PROPERTY "${_property_prefix}_LINKS" "")
  set_property(GLOBAL PROPERTY "${_property_prefix}_INCLUDES" "")
  set_property(GLOBAL PROPERTY "${_property_prefix}_DEFINITIONS" "")
  set_property(GLOBAL PROPERTY "${_property_prefix}_VISITED" "")
  foreach(_root_target IN LISTS ARGN)
    _winux_collect_static_libraries("${_root_target}" "${_scope}")
  endforeach()

  get_property(_libraries GLOBAL PROPERTY "${_property_prefix}_LIBRARIES")
  get_property(_external_links GLOBAL PROPERTY "${_property_prefix}_LINKS")
  get_property(_includes GLOBAL PROPERTY "${_property_prefix}_INCLUDES")
  get_property(_definitions GLOBAL PROPERTY "${_property_prefix}_DEFINITIONS")

  if(NOT _libraries)
    message(FATAL_ERROR "No static library targets were found to merge into '${target_name}'.")
  endif()

  if(_includes)
    list(REMOVE_DUPLICATES _includes)
    target_include_directories("${target_name}" PRIVATE ${_includes})
  endif()

  if(_definitions)
    list(REMOVE_DUPLICATES _definitions)
    target_compile_definitions("${target_name}" PRIVATE ${_definitions})
  endif()

  if(_external_links)
    list(REMOVE_DUPLICATES _external_links)
    target_link_libraries("${target_name}" PRIVATE ${_external_links})
  endif()

  list(REMOVE_DUPLICATES _libraries)
  set(_merge_arguments)
  set(_index 0)
  foreach(_library IN LISTS _libraries)
    math(EXPR _index "${_index} + 1")
    list(APPEND _merge_arguments "-DWINUX_MERGE_LIBRARY_${_index}=$<TARGET_FILE:${_library}>")
  endforeach()

  if(WIN32)
    set(_merge_platform "WINDOWS")
  else()
    set(_merge_platform "UNIX")
  endif()

  add_custom_command(TARGET "${target_name}" POST_BUILD
    COMMAND "${CMAKE_COMMAND}"
      "-DWINUX_MERGE_ARCHIVE=$<TARGET_FILE:${target_name}>"
      "-DWINUX_MERGE_TOOL=${CMAKE_AR}"
      "-DWINUX_MERGE_PLATFORM=${_merge_platform}"
      "-DWINUX_MERGE_COUNT=${_index}"
      ${_merge_arguments}
      -P "${_WINUX_STATIC_LIB_MERGE_SCRIPT}"
    COMMENT "Merging static dependencies into ${target_name}"
    VERBATIM
  )
endfunction()

set(_WINUX_STATIC_LIB_MERGE_SCRIPT "${CMAKE_CURRENT_LIST_DIR}/StaticLibMerge.cmake")

if(DEFINED WINUX_MERGE_ARCHIVE)
  if(NOT EXISTS "${WINUX_MERGE_ARCHIVE}")
    message(FATAL_ERROR "Winux archive does not exist: ${WINUX_MERGE_ARCHIVE}")
  endif()

  set(_merge_libraries)
  foreach(_index RANGE 1 ${WINUX_MERGE_COUNT})
    if(NOT EXISTS "${WINUX_MERGE_LIBRARY_${_index}}")
      message(FATAL_ERROR "Static dependency archive does not exist: ${WINUX_MERGE_LIBRARY_${_index}}")
    endif()
    list(APPEND _merge_libraries "${WINUX_MERGE_LIBRARY_${_index}}")
  endforeach()

  set(_temporary_archive "${WINUX_MERGE_ARCHIVE}.merged")
  file(REMOVE "${_temporary_archive}")

  if(WINUX_MERGE_PLATFORM STREQUAL "WINDOWS")
    execute_process(
      COMMAND "${WINUX_MERGE_TOOL}" /NOLOGO "/OUT:${_temporary_archive}"
        "${WINUX_MERGE_ARCHIVE}" ${_merge_libraries}
      RESULT_VARIABLE _merge_result
      OUTPUT_VARIABLE _merge_output
      ERROR_VARIABLE _merge_error
    )
  else()
    set(_mri_script "${_temporary_archive}.mri")
    set(_mri_contents "create \"${_temporary_archive}\"\naddlib \"${WINUX_MERGE_ARCHIVE}\"\n")
    foreach(_library IN LISTS _merge_libraries)
      string(APPEND _mri_contents "addlib \"${_library}\"\n")
    endforeach()
    string(APPEND _mri_contents "save\nend\n")
    file(WRITE "${_mri_script}" "${_mri_contents}")

    execute_process(
      COMMAND "${WINUX_MERGE_TOOL}" -M
      INPUT_FILE "${_mri_script}"
      RESULT_VARIABLE _merge_result
      OUTPUT_VARIABLE _merge_output
      ERROR_VARIABLE _merge_error
    )
    file(REMOVE "${_mri_script}")
    if(_merge_result EQUAL 0)
      execute_process(
        COMMAND "${WINUX_MERGE_TOOL}" s "${_temporary_archive}"
        RESULT_VARIABLE _ranlib_result
        OUTPUT_VARIABLE _ranlib_output
        ERROR_VARIABLE _ranlib_error
      )
      if(NOT _ranlib_result EQUAL 0)
        set(_merge_result "${_ranlib_result}")
        set(_merge_error "${_ranlib_error}")
      endif()
    endif()
  endif()

  if(NOT _merge_result EQUAL 0)
    file(REMOVE "${_temporary_archive}")
    message(FATAL_ERROR "Static archive merge failed: ${_merge_output}${_merge_error}")
  endif()

  execute_process(
    COMMAND "${CMAKE_COMMAND}" -E copy "${_temporary_archive}" "${WINUX_MERGE_ARCHIVE}"
    RESULT_VARIABLE _copy_result
    ERROR_VARIABLE _copy_error
  )
  file(REMOVE "${_temporary_archive}")
  if(NOT _copy_result EQUAL 0)
    message(FATAL_ERROR "Could not replace Winux archive after merging: ${_copy_error}")
  endif()
endif()
# - Find gettext
# This module looks for the gettext-tools, which are used to simplify the
# translation of a software package. It is used by including the UseGettext.cmake
# module.
#
# It defines the following macros:
#
#  GETTEXT_CREATE_POT_FILE(potfilename [ALL] [sources...])
#      creates the initial pot file. It is basically a wrapper around
#      the xgettext executable. If ALL is specified, all source files
#      of the project are used.
#
#  GETTEXT_ADD_PO_FILES(potfilename [ALL] [WITH_LL] lang...])
#      This macro collects all po files for the given languages and adds
#      them to the list of source files for the potfile. If ALL is given
#      all available translations are used. If WITH_LL is given, not only
#      xx.po is considered, but also xx_LL.po files.
#
#  GETTEXT_PROCESS_PO_FILES(potfilename [ALL] [INSTALL_DESTINATION dest]
#                           [languages...])
#      This does the actual processing of the po files. It creates the
#      gmo files and adds a custom target to rebuild them when the po
#      files change. If INSTALL_DESTINATION is given, it also adds an install
#      target for the gmo files.
#
#  GETTEXT_PROCESS_POT_FILE(potfilename [ALL] [languages...])
#      This macro updates the po files from the pot file.
#
# It is used by including the UseGettext.cmake module.
#
#=============================================================================
# Copyright 2007-2009 Alexander Neundorf, <neundorf@kde.org>
#
# Distributed under the OSI-approved BSD License (the "License");
# see accompanying file Copyright.txt for details.
#
# This software is distributed WITHOUT ANY WARRANTY; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
# See the License for more information.
#=============================================================================
# (To distribute this file outside of CMake, substitute the full
#  License text for the above reference.)
# first find the gettext package, which contains the tools
find_package(Gettext REQUIRED)

# =============================================================================
# 1. Define default GETTEXT_PO_DIR if not set by user
# =============================================================================
if(NOT GETTEXT_PO_DIR)
    set(GETTEXT_PO_DIR ${CMAKE_CURRENT_SOURCE_DIR}/po)
endif()

# =============================================================================
# 2. GETTEXT_PROCESS_POT_FILE: Update .po files from .pot
# =============================================================================
macro(GETTEXT_PROCESS_POT_FILE pot_file)
   set(langs ${ARGN})
   if("${langs}" STREQUAL "ALL")
      file(GLOB langs RELATIVE ${GETTEXT_PO_DIR} ${GETTEXT_PO_DIR}/*.po)
      string(REGEX REPLACE "\\.po" "" langs "${langs}")
   endif("${langs}" STREQUAL "ALL")
   foreach(lang ${langs})
      add_custom_command(OUTPUT ${GETTEXT_PO_DIR}/${lang}.po
         COMMAND msgmerge -U ${GETTEXT_PO_DIR}/${lang}.po ${GETTEXT_PO_DIR}/${pot_file}.pot
         DEPENDS ${GETTEXT_PO_DIR}/${pot_file}.pot
         WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
         COMMENT "Updating ${lang}.po")
      list(APPEND po_files ${GETTEXT_PO_DIR}/${lang}.po)
   endforeach(lang)
   add_custom_target(update_po ALL DEPENDS ${po_files})
endmacro(GETTEXT_PROCESS_POT_FILE)

# =============================================================================
# 3. GETTEXT_CREATE_POT_FILE: Generate .pot from source files
# =============================================================================
macro(GETTEXT_CREATE_POT_FILE pot_file)
   # collect the files
   set(source_files "")
   set(files ${ARGN})
   if("${files}" STREQUAL "ALL")
      get_target_property(source_files ${PROJECT_NAME} SOURCES)
   else()
      set(source_files ${files})
   endif()
   GETTEXT_FIND_KEYWORDS(keywords)
   # create a temporary list file for xgettext
   set(tmp_file_list ${CMAKE_CURRENT_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/xgettext_list_file.txt)
   string(REPLACE ";" "
" file_list_content "${source_files}")
   file(WRITE ${tmp_file_list} "${file_list_content}")
   file(MAKE_DIRECTORY ${GETTEXT_PO_DIR})  # ✅ Usa GETTEXT_PO_DIR
   set(GETTEXT_POT_FILE ${GETTEXT_PO_DIR}/${pot_file}.pot)  # ✅ Usa GETTEXT_PO_DIR
   add_custom_command(OUTPUT ${GETTEXT_POT_FILE}
      COMMAND xgettext ${GETTEXT_XGETTEXT_FLAGS} --files-from=${tmp_file_list} -o ${GETTEXT_POT_FILE} ${keywords}
      DEPENDS ${source_files} ${tmp_file_list}
      WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
      COMMENT "Creating template file ${pot_file}.pot"
      )
   add_custom_target(potfile ALL DEPENDS ${GETTEXT_POT_FILE})
endmacro(GETTEXT_CREATE_POT_FILE)

# =============================================================================
# 4. GETTEXT_ADD_PO_FILES: Register .po files as sources and dependencies
# =============================================================================
macro(GETTEXT_ADD_PO_FILES pot_file)
   set(langs "")
   set(do_ll FALSE)
   foreach(arg ${ARGN})
      if("${arg}" STREQUAL "ALL")
         if(do_ll)
            file(GLOB lang_files RELATIVE ${GETTEXT_PO_DIR} ${GETTEXT_PO_DIR}/*.po ${GETTEXT_PO_DIR}/*_*.po)
         else()
            file(GLOB lang_files RELATIVE ${GETTEXT_PO_DIR} ${GETTEXT_PO_DIR}/*.po)
         endif()
         list(APPEND langs ${lang_files})
         set(do_ll FALSE)
      elseif("${arg}" STREQUAL "WITH_LL")
         set(do_ll TRUE)
      else()
         list(APPEND langs ${arg}.po)
         if(do_ll)
            file(GLOB ll_files RELATIVE ${GETTEXT_PO_DIR} ${GETTEXT_PO_DIR}/${arg}_*.po)
            list(APPEND langs ${ll_files})
         endif()
         set(do_ll FALSE)
      endif()
   endforeach()
   set(po_dir ${GETTEXT_PO_DIR})  # ✅ Usa GETTEXT_PO_DIR
   foreach(lang_file ${langs})
       set(full_path_lang_file ${po_dir}/${lang_file})
       set_source_files_properties(${full_path_lang_file} PROPERTIES HEADER_FILE_ONLY TRUE)
       set_property(SOURCE ${full_path_lang_file} APPEND PROPERTY OBJECT_DEPENDS ${po_dir}/${pot_file}.pot)
   endforeach()
endmacro(GETTEXT_ADD_PO_FILES)

# =============================================================================
# 5. GETTEXT_PROCESS_PO_FILES: Compile .po to .mo and install
# =============================================================================
macro(GETTEXT_PROCESS_PO_FILES pot_file)
   set(langs "")
   set(dest "")
   set(next_is_dest FALSE)
   foreach(arg ${ARGN})
      if(next_is_dest)
         set(dest ${arg})
         set(next_is_dest FALSE)
      else()
         if("${arg}" STREQUAL "ALL")
            file(GLOB langs RELATIVE ${GETTEXT_PO_DIR} ${GETTEXT_PO_DIR}/*.po)
            string(REGEX REPLACE "\\.po" "" langs "${langs}")
         elseif("${arg}" STREQUAL "INSTALL_DESTINATION")
            set(next_is_dest TRUE)
         else()
            list(APPEND langs ${arg})
         endif()
      endif()
   endforeach()
   foreach(lang ${langs})
      set(po_file ${GETTEXT_PO_DIR}/${lang}.po)  # ✅ Usa GETTEXT_PO_DIR
      # Create correct locale structure in build dir
      set(gmo_dir ${CMAKE_CURRENT_BINARY_DIR}/locale/${lang}/LC_MESSAGES)
      file(MAKE_DIRECTORY ${gmo_dir})
      set(gmo_file ${gmo_dir}/${pot_file}.mo)
      add_custom_command(OUTPUT ${gmo_file}
         COMMAND msgfmt -o ${gmo_file} ${po_file}
         DEPENDS ${po_file}
         COMMENT "Compiling ${po_file} to ${gmo_file}"
         )
      list(APPEND gmo_files ${gmo_file})
      if(dest)
         install(FILES ${gmo_file} DESTINATION ${dest}/${lang}/LC_MESSAGES RENAME ${pot_file}.mo)
      endif()
   endforeach(lang)
   add_custom_target(translations ALL DEPENDS ${gmo_files})
endmacro(GETTEXT_PROCESS_PO_FILES)

# =============================================================================
# 6. GETTEXT_FIND_KEYWORDS: Define gettext keywords
# =============================================================================
macro(GETTEXT_FIND_KEYWORDS keywords)
   set(my_keywords)
   if(GETTEXT_KEYWORDS)
      foreach(kw ${GETTEXT_KEYWORDS})
         set(my_keywords ${my_keywords} --keyword=${kw})
      endforeach()
   else()
      set(my_keywords ${my_keywords} --keyword=_)
   endif()
   set(${keywords} ${my_keywords})
endmacro(GETTEXT_FIND_KEYWORDS)

file(GLOB QML_JS_FILES *.qml *.js)

# make the files visible on qtcreator
add_custom_target(%ProjectName:l%_QMlFiles ALL SOURCES ${QML_JS_FILES})

#substitute variables in the desktop file
configure_file(${DESKTOP_FILE_NAME}.in ${CMAKE_CURRENT_BINARY_DIR}/${DESKTOP_FILE_NAME}.in)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${DESKTOP_FILE_NAME} DESTINATION ${DESKTOP_DIR})
install(FILES ${QML_JS_FILES} DESTINATION ${%ProjectName:u%_DIR})

add_subdirectory(components)
add_subdirectory(ui)

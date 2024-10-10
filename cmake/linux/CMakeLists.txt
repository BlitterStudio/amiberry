# Copy dirs to the build directory so we can debug locally
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/controllers
        $<TARGET_FILE_DIR:${PROJECT_NAME}>/controllers)

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/data
        $<TARGET_FILE_DIR:${PROJECT_NAME}>/data)

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/roms
        $<TARGET_FILE_DIR:${PROJECT_NAME}>/roms)

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/whdboot
        $<TARGET_FILE_DIR:${PROJECT_NAME}>/whdboot)

install(FILES $<TARGET_FILE:capsimage>
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/${PROJECT_NAME})
install(FILES $<TARGET_FILE:floppybridge>
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/${PROJECT_NAME})

# This one contains the gamecontrollersdb.txt file
install(DIRECTORY ${CMAKE_SOURCE_DIR}/controllers
        DESTINATION ${CMAKE_INSTALL_DATADIR}/${PROJECT_NAME})
# This one contains the data files
install(DIRECTORY ${CMAKE_SOURCE_DIR}/data
        DESTINATION ${CMAKE_INSTALL_DATADIR}/${PROJECT_NAME})
# This one contains the AROS kickstart files
install(DIRECTORY ${CMAKE_SOURCE_DIR}/roms
        DESTINATION ${CMAKE_INSTALL_DATADIR}/${PROJECT_NAME})
# This one contains the whdboot files
install(DIRECTORY ${CMAKE_SOURCE_DIR}/whdboot
        DESTINATION ${CMAKE_INSTALL_DATADIR}/${PROJECT_NAME})
# Install desktop file
install(FILES ${CMAKE_SOURCE_DIR}/packaging/Amiberry.desktop
        DESTINATION ${CMAKE_INSTALL_DATADIR}/applications)
# Install icon
install(FILES ${CMAKE_SOURCE_DIR}/data/amiberry.svg
        DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/scalable/apps)

install(FILES ${CMAKE_SOURCE_DIR}/debian/changelog.gz
        DESTINATION ${CMAKE_INSTALL_DOCDIR})
install(FILES ${CMAKE_SOURCE_DIR}/debian/copyright
        DESTINATION ${CMAKE_INSTALL_DOCDIR})
install(FILES ${CMAKE_SOURCE_DIR}/packaging/linux/man/amiberry.1.gz
        DESTINATION ${CMAKE_INSTALL_MANDIR}/man1)
install(FILES ${CMAKE_SOURCE_DIR}/packaging/linux/Amiberry.metainfo.xml
        DESTINATION ${CMAKE_INSTALL_DATADIR}/metainfo)
install(FILES ${CMAKE_SOURCE_DIR}/packaging/linux/mime/amiberry.xml
        DESTINATION ${CMAKE_INSTALL_DATADIR}/mime/packages)
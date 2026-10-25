# iOS install rules
# iOS .app bundles are flat: the executable and resources are siblings
# at the bundle root. $<TARGET_FILE_DIR:x> = the .app/ directory itself.

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/controllers
        $<TARGET_FILE_DIR:${PROJECT_NAME}>/controllers)

# The privacy manifest the App Store requires, at the bundle root.
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/packaging/PrivacyInfo.xcprivacy
        $<TARGET_FILE_DIR:${PROJECT_NAME}>/PrivacyInfo.xcprivacy)

# Third-party licences and the GPL source offer. The iOS bundle is flat.
# The offer is generated at build time (always-run target) so it names the
# revision this binary was built from; the copy is its own always-run target
# so it refreshes without a relink, before the POST_BUILD steps.
set(AMIBERRY_LICENCES_DIR $<TARGET_FILE_DIR:${PROJECT_NAME}>/licences)
# Computed here, not through $<BOOL:${VERSION_PRE_RELEASE}>: a non-numeric
# pre-release string ("beta1") would make that generator expression false.
# Xcode signs the app at the end of the target build, and this bundle step
# is a target dependency, so the copy always lands before signing.
if(VERSION_PRE_RELEASE)
    set(AMIBERRY_PRERELEASE_FLAG 1)
else()
    set(AMIBERRY_PRERELEASE_FLAG 0)
endif()
add_custom_target(amiberry_licences_offer
        COMMAND ${CMAKE_COMMAND}
        -DSRC_DIR=${CMAKE_SOURCE_DIR}
        -DOUT_FILE=${CMAKE_BINARY_DIR}/amiberry-source-offer.json
        -DAMIBERRY_PRODUCT_VERSION=${AMIBERRY_PRODUCT_VERSION}
        -DPRERELEASE=${AMIBERRY_PRERELEASE_FLAG}
        -DAMIBERRY_DISTRIBUTION=${AMIBERRY_DISTRIBUTION}
        -P ${CMAKE_SOURCE_DIR}/cmake/SourceOffer.cmake)
add_custom_target(amiberry_licences_bundle ALL
        COMMAND ${CMAKE_COMMAND}
        -DSRC_DIR=${CMAKE_SOURCE_DIR}/packaging/licences
        -DDEST_DIR=${AMIBERRY_LICENCES_DIR}
        -DDISTRIBUTION=${AMIBERRY_DISTRIBUTION}
        -DOFFER_FILE=${CMAKE_BINARY_DIR}/amiberry-source-offer.json
        -P ${CMAKE_SOURCE_DIR}/cmake/LicencesBundle.cmake)
add_dependencies(amiberry_licences_bundle amiberry_licences_offer)
add_dependencies(${PROJECT_NAME} amiberry_licences_bundle)

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

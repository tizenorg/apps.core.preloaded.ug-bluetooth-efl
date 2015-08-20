SET(TARGET "theme_2.3")

ADD_CUSTOM_TARGET(bluetooth_images.edj
                COMMAND edje_cc -id ${CMAKE_CURRENT_SOURCE_DIR}/images/WVGA
		        ${CMAKE_CURRENT_SOURCE_DIR}/${TARGET}/bluetooth_images.edc
			${CMAKE_CURRENT_SOURCE_DIR}/${TARGET}/bluetooth_images.edj
		DEPENDS
			${CMAKE_CURRENT_SOURCE_DIR}/${TARGET}/bluetooth_images.edc
		)
ADD_CUSTOM_TARGET(bluetooth_genlist.edj
                COMMAND edje_cc -id ${CMAKE_CURRENT_SOURCE_DIR}/images/WVGA
                        ${CMAKE_CURRENT_SOURCE_DIR}/${TARGET}/bluetooth_genlist.edc
                        ${CMAKE_CURRENT_SOURCE_DIR}/${TARGET}/bluetooth_genlist.edj
                DEPENDS
                        ${CMAKE_CURRENT_SOURCE_DIR}/${TARGET}/bluetooth_genlist.edc
		)

ADD_DEPENDENCIES(${PROJECT_NAME} bluetooth_images.edj)
ADD_DEPENDENCIES(${PROJECT_NAME} bluetooth_genlist.edj)

INSTALL(FILES ${CMAKE_CURRENT_SOURCE_DIR}/${TARGET}/bluetooth_images.edj DESTINATION ${EDJDIR})
INSTALL(FILES ${CMAKE_CURRENT_SOURCE_DIR}/${TARGET}/bluetooth_genlist.edj DESTINATION ${EDJDIR})

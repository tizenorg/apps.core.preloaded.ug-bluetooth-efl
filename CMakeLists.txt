CMAKE_MINIMUM_REQUIRED(VERSION 2.6)
PROJECT(ug-setting-bluetooth-efl C)

SET(PREFIX ${CMAKE_INSTALL_PREFIX})
SET(EXEC_PREFIX "\${prefix}")
SET(LIBDIR "\${prefix}/lib")
SET(INCLUDEDIR "\${prefix}/include")
SET(VERSION 0.1.0)
SET(RESDIR "${PREFIX}/res")
SET(LOCALEDIR "${RESDIR}/locale")
SET(EDJDIR "${RESDIR}/edje/${PROJECT_NAME}")
SET(IMAGEDIR "${RESDIR}/images/${PROJECT_NAME}")
SET(HELPDIR "${RESDIR}/help/${PROJECT_NAME}")
SET(TABLEDIR "${RESDIR}/tables/${PROJECT_NAME}")

SET(SRCS
src/ui/bt-main-ug.c
src/ui/bt-widget.c
src/ui/bt-main-view.c
src/ui/bt-profile-view.c
src/libraries/bt-util.c
src/libraries/bt-callback.c
src/libraries/bt-ipc-handler.c
src/libraries/bt-net-connection.c
)

INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR}/include)

SET(PKG_MODULES
	elementary
	appcore-efl
	ui-gadget-1
	dlog
	vconf
	edbus
	evas
	edje
	ecore
	eina
	aul
	syspopup-caller
	gobject-2.0
	motion
	capi-network-bluetooth
	capi-network-connection
	capi-system-device
	capi-appfw-application
	notification
	efl-extension
	glib-2.0
	gio-2.0
)

INCLUDE(FindPkgConfig)
pkg_check_modules(pkgs REQUIRED ${PKG_MODULES})

FOREACH(flag ${pkgs_CFLAGS})
	SET(EXTRA_CXXFLAGS "${EXTRA_CXXFLAGS} ${flag}")
ENDFOREACH(flag)

SET(GC_SECTIONS_FLAGS "-fdata-sections -ffunction-sections -Wl,--gc-sections")
SET(EXTRA_CXXFLAGS "${EXTRA_CXXFLAGS} -fvisibility=hidden -Wall")
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${EXTRA_CXXFLAGS} ${GC_SECTIONS_FLAGS}")
SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${CMAKE_CXX_FLAGS} ${GC_SECTIONS_FLAGS} -fPIC -Werror")

ADD_DEFINITIONS("-DPREFIX=\"${CMAKE_INSTALL_PREFIX}\"")
ADD_DEFINITIONS("-DEDJDIR=\"${EDJDIR}\"")
ADD_DEFINITIONS("-DRESDIR=\"${RESDIR}\"")
ADD_DEFINITIONS("-DIMAGEDIR=\"${IMAGEDIR}\"")
ADD_DEFINITIONS("-DBT_USE_PLATFORM_DBG")
#ADD_DEFINITIONS("-D__TIZEN_OPEN__")

INSTALL(FILES ${CMAKE_BINARY_DIR}/data/ug-bluetooth-efl.xml DESTINATION /usr/share/packages/)
INSTALL(FILES ${CMAKE_BINARY_DIR}/data/images/ug-bluetooth-efl.png DESTINATION /usr/share/icons/default/small)

ADD_LIBRARY(${PROJECT_NAME} SHARED ${SRCS})
SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES SOVERSION 0.1.0)
TARGET_LINK_LIBRARIES(${PROJECT_NAME} ${pkgs_LDFLAGS})

INSTALL(TARGETS ${PROJECT_NAME} DESTINATION /usr/ug/lib)

INSTALL(FILES ${CMAKE_CURRENT_SOURCE_DIR}/data/images/ug-bluetooth-efl.png DESTINATION ${IMAGEDIR})
INSTALL(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/data/images/ DESTINATION ${IMAGEDIR} FILES_MATCHING PATTERN "*.png")
INSTALL(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/data/images/ DESTINATION ${IMAGEDIR} FILES_MATCHING PATTERN "*.jpg")
INSTALL(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/data/help/ DESTINATION ${HELPDIR} FILES_MATCHING PATTERN "*help_*")
INSTALL(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/tables/WVGA/ DESTINATION ${TABLEDIR} FILES_MATCHING PATTERN "*.xml")


ADD_SUBDIRECTORY(po)
ADD_SUBDIRECTORY(data)
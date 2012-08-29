# for i18n

SET(POFILES
ar.po az.po ca.po da.po el_GR.po en_PH.po en_US.po es_US.po et.po eu.po
fi.po ga.po gl.po hr.po hy.po is.po ja_JP.po ka.po kk.po lt.po
pt_PT.po sl.po sv.po tr_TR.po zh_HK.po mk.po nb.po pt_BR.po ru_RU.po bg.po
cs.po de_DE.po en.po es_ES.po fr_FR.po fr_CA.po hi.po hu.po it_IT.po ko_KR.po
lv.po nl_NL.po pl.po ro.po sk.po sr.po uk.po uz.po zh_CN.po zh_TW.po
zh_SG.po)

SET(MSGFMT "/usr/bin/msgfmt")

FOREACH(pofile ${POFILES})
	SET(pofile ${CMAKE_CURRENT_SOURCE_DIR}/${pofile})
	MESSAGE("PO: ${pofile}")
	GET_FILENAME_COMPONENT(absPofile ${pofile} ABSOLUTE)
	GET_FILENAME_COMPONENT(lang ${absPofile} NAME_WE)
	SET(moFile ${CMAKE_CURRENT_BINARY_DIR}/${lang}.mo)
	ADD_CUSTOM_COMMAND(
			OUTPUT ${moFile}
			COMMAND ${MSGFMT} -o ${moFile} ${absPofile}
			DEPENDS ${absPofile}
	)
	INSTALL(FILES ${moFile}
			DESTINATION ${LOCALEDIR}/${lang}/LC_MESSAGES RENAME ${PROJECT_NAME}.mo)
	SET(moFiles ${moFiles} ${moFile})
ENDFOREACH(pofile)

MESSAGE(".mo files: ${moFiles}")
ADD_CUSTOM_TARGET(po ALL DEPENDS ${moFiles})

FILE(GLOB desktop_files "*.desktop")
FOREACH (file ${desktop_files})
     INSTALL(FILES ${file} DESTINATION ${KDE_INSTALL_DATADIR}/skrooge/currency)
ENDFOREACH()

project( MantidParaViewMDHWNexusReader )

add_paraview_plugin( MantidParaViewMDHWNexusReaderSMPlugin "1.0"
	SERVER_MANAGER_XML MDHWNexusReader.xml
	SERVER_MANAGER_SOURCES vtkMDHWNexusReader.cxx
	GUI_RESOURCE_FILES MDHWNexusReaderGUI.xml
)

# Add to the 'VatesParaViewPlugins' group in VS
set_property( TARGET MantidParaViewMDHWNexusReaderSMPlugin PROPERTY FOLDER "MantidVatesParaViewPlugins" )

target_link_libraries( MantidParaViewMDHWNexusReaderSMPlugin 
${MANTID_SUBPROJECT_LIBS} )

# Put library into subfolder.
SET_TARGET_OUTPUT_DIRECTORY(MantidParaViewMDHWNexusReaderSMPlugin ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/${CMAKE_CFG_INTDIR}/${PVPLUGINS_DIR}/${PVPLUGINS_SUBDIR})

install( TARGETS MantidParaViewMDHWNexusReaderSMPlugin ${SYSTEM_PACKAGE_TARGET} DESTINATION ${PVPLUGINS_DIR}/${PVPLUGINS_SUBDIR} )



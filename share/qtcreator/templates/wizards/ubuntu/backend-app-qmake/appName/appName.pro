TEMPLATE = aux
TARGET = %ClickHookName%

RESOURCES += %ClickHookName%.qrc

QML_FILES += $$files(*.qml,true) \
					   $$files(*.js,true)

CONF_FILES +=  %ClickHookName%.apparmor \
               %ClickHookName%.desktop \
               %ClickHookName%.png

OTHER_FILES += $${CONF_FILES} \
               $${QML_FILES}

#specify where the qml/js files are installed to
qml_files.path = /%ClickHookName%
qml_files.files += $${QML_FILES}

#specify where the config files are installed to
config_files.path = /%ClickHookName%
config_files.files += $${CONF_FILES}

INSTALLS+=config_files qml_files

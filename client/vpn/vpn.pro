TEMPLATE = subdirs
CONFIG      += ordered
SUBDIRS += obhod123 \
           enctester

macx {
    SUBDIRS += macOSLauncher macOSCompose8
}

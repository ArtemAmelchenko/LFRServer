HEADERS += \
    lfrconnection.h \
    lfrconnectionsmanager.h \
    personalcardmanager.h

SOURCES += \
    lfrconnection.cpp \
    lfrconnectionsmanager.cpp \
    main.cpp \
    personalcardmanager.cpp

unix:!macx: LIBS += -L$$PWD/../../boost_1_79_0/lib/lib/ -lboost_thread

INCLUDEPATH += $$PWD/../../boost_1_79_0/lib/include
DEPENDPATH += $$PWD/../../boost_1_79_0/lib/include


unix:!macx: LIBS += -L$$PWD/../../boost_1_79_0/lib/lib/ -lboost_log

INCLUDEPATH += $$PWD/../../boost_1_79_0/lib/include
DEPENDPATH += $$PWD/../../boost_1_79_0/lib/include


unix:!macx: LIBS += -L$$PWD/../../boost_1_79_0/lib/lib/ -lboost_log_setup

INCLUDEPATH += $$PWD/../../boost_1_79_0/lib/include
DEPENDPATH += $$PWD/../../boost_1_79_0/lib/include


unix:!macx: LIBS += -L$$PWD/../../boost_1_79_0/lib/lib/ -lboost_filesystem

INCLUDEPATH += $$PWD/../../boost_1_79_0/lib/include
DEPENDPATH += $$PWD/../../boost_1_79_0/lib/include



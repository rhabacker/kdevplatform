add_definitions(-DTRANSLATION_DOMAIN=\"kdevwelcomepage\")
add_subdirectory(declarative)

set(welcomepage_SRCS
    welcomepageplugin.cpp
    sessionsmodel.cpp
    welcomepageview.cpp
    uihelper.cpp)

qt5_add_resources(welcomepage_SRCS welcomepage.qrc)

kdevplatform_add_plugin(kdevwelcomepage JSON kdevwelcomepage.json SOURCES ${welcomepage_SRCS})

target_link_libraries(kdevwelcomepage
    KDev::Interfaces
    KDev::Sublime
    KDev::Shell
    KDev::Project
    Qt5::QuickWidgets
    KF5::Declarative
)

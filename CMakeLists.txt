# Минимальная версия CMake
cmake_minimum_required(VERSION 3.16)

# Название проекта, версия и язык
project(Library VERSION 0.1 LANGUAGES CXX)

# Включаем автоматическую обработку MOC, UIC, RCC
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# Устанавливаем стандарт C++
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Explicitly tell AUTOUIC where to find headers for custom widgets
set(CMAKE_AUTOUIC_SEARCH_PATHS ${CMAKE_CURRENT_SOURCE_DIR})

# --- Поиск Qt ---
# Ищем Qt6 или Qt5 и запрашиваем ВСЕ необходимые компоненты СРАЗУ.
# Это установит QT_VERSION_MAJOR и другие переменные.
find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core Sql Widgets)
# Используем найденную версию для последующих команд Qt
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Sql Widgets)

# --- Список исходных файлов ---
# Перечисляем .cpp и .h файлы. .ui и .qrc будут добавлены ниже.
set(PROJECT_SOURCES
    main.cpp
    starratingwidget.cpp
    starratingwidget.h
    mainwindow.cpp
    mainwindow.h
    database.cpp
    database.h
    logindialog.cpp
    logindialog.h
    profiledialog.cpp # Додано новий файл діалогу профілю
    profiledialog.h
)

# --- Создание исполняемого файла ---
# Используем разный синтаксис для Qt 6 и Qt 5
if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    # qt_add_executable (рекомендуется для Qt 6)
    # Передаем .ui и .qrc файлы напрямую
    qt_add_executable(untitled
        MANUAL_FINALIZATION # Нужно, если используем qt_finalize_executable позже
        ${PROJECT_SOURCES}
        mainwindow.ui
        logindialog.ui # Додано UI файл діалогу входу
        profiledialog.ui # Додано UI файл діалогу профілю
        resourses.qrc
        # starratingwidget.h/cpp removed here, already in PROJECT_SOURCES
    )
else()
    # add_executable (стандартный CMake, используется для Qt 5)
    # CMAKE_AUTOUIC и CMAKE_AUTORCC должны автоматически обработать
    # mainwindow.ui и resourses.qrc, если они используются в ${PROJECT_SOURCES}
    if(ANDROID) # Сохраняем логику для Android, если она нужна
        add_library(untitled SHARED
            ${PROJECT_SOURCES}
            # Для Qt5 AUTOUIC/RCC должны подхватить .ui/.qrc из исходников
        )
    else()
        add_executable(untitled
            ${PROJECT_SOURCES}
            # Для Qt5 AUTOUIC/RCC должны подхватить .ui/.qrc из исходников
        )
    endif()
endif()

# --- Линковка с библиотеками Qt ---
# Линкуем со ВСЕМИ необходимыми компонентами Qt
target_link_libraries(untitled PRIVATE
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Sql     # <--- Добавлено!
    Qt${QT_VERSION_MAJOR}::Widgets
)

# --- Настройки для платформ (macOS/iOS/Windows) ---
# (Оставляем как было, это стандартный шаблон)
if(${QT_VERSION} VERSION_LESS 6.1.0 AND (APPLE OR CMAKE_SYSTEM_NAME STREQUAL "iOS"))
    set(BUNDLE_ID_OPTION MACOSX_BUNDLE_GUI_IDENTIFIER com.example.untitled)
endif()
set_target_properties(untitled PROPERTIES
    ${BUNDLE_ID_OPTION}
    MACOSX_BUNDLE TRUE
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    WIN32_EXECUTABLE TRUE
)

# --- Установка ---
# (Оставляем как было, это стандартный шаблон)
include(GNUInstallDirs)
install(TARGETS untitled
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

# --- Финализация для Qt 6 ---
# (Оставляем как было, парная команда для MANUAL_FINALIZATION)
if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(untitled)
endif()

# --- (Опционально, но рекомендуется) Добавление путей для заголовочных файлов ---
# Если структура проекта сложная, это может помочь IDE и компилятору
# target_include_directories(untitled PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

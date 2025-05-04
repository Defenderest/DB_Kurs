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

# Добавляем текущую директорию в пути поиска заголовочных файлов ДО определения исполняемого файла
include_directories(${CMAKE_CURRENT_SOURCE_DIR})

# --- Список исходных файлов ---
# Перечисляем .cpp та .h файли.
set(PROJECT_SOURCES
    main.cpp
    starratingwidget.cpp
    starratingwidget.h
    mainwindow.cpp
    mainwindow.h
    database.h # Повертаємо заголовковий файл для DatabaseManager
    database_connection.cpp
    database_customer.cpp
    database_book.cpp
    database_author.cpp
    database_order.cpp
    database_comment.cpp
    database_cart.cpp
    logindialog.cpp
    logindialog.h
    profiledialog.cpp
    profiledialog.h
    testdata.cpp
    testdata.h
    datatypes.h # Заголовковий файл зі структурами
    mainwindow_utils.cpp
    mainwindow_books.cpp
    mainwindow_authors.cpp
    mainwindow_orders.cpp
    mainwindow_profile.cpp
    mainwindow_cart.cpp
    mainwindow_comments.cpp
    mainwindow_search.cpp
    searchsuggestiondelegate.cpp
    searchsuggestiondelegate.h
    RangeSlider.cpp
    RangeSlider.h
    checkoutdialog.cpp
    checkoutdialog.h
)

# --- Создание исполняемого файла ---
# Используем разный синтаксис для Qt 6 и Qt 5
if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    # qt_add_executable (рекомендуется для Qt 6)
    # Передаем .ui и .qrc файлы напрямую
    qt_add_executable(untitled
        MANUAL_FINALIZATION # Нужно, если используем qt_finalize_executable позже
        ${PROJECT_SOURCES} # Тільки .cpp файли
        # Заголовки (.h) не передаємо, вони знаходяться через include_directories
        # UI та QRC файли:
        mainwindow.ui
        logindialog.ui
        profiledialog.ui
        checkoutdialog.ui
        resourses.qrc
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

# --- Копирование SQL файлов в директорию сборки ---
# Это гарантирует, что программа найдет их при запуске из IDE/сборки
file(COPY sql DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
message(STATUS "Copying SQL files to build directory: ${CMAKE_CURRENT_BINARY_DIR}/sql")


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

# --- Встановлення SQL файлів ---
# Копіюємо SQL файли до піддиректорії 'sql' в директорії встановлення виконуваного файлу
install(FILES
    sql/schema.sql
    sql/author_queries.sql
    sql/book_queries.sql
    sql/cart_queries.sql
    sql/comment_queries.sql
    sql/customer_queries.sql
    sql/order_queries.sql
    DESTINATION ${CMAKE_INSTALL_BINDIR}/sql
)

# --- Финализация для Qt 6 ---
# (Оставляем как было, парная команда для MANUAL_FINALIZATION)
if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(untitled)
endif()


# --- (Опционально) Добавление путей для заголовочных файлов через target_include_directories ---
# Можна залишити або видалити, оскільки include_directories() вже додано вище.
# target_include_directories(untitled PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})


# --- Додавання залежностей для компіляції (якщо потрібно) ---
# Наприклад, якщо нові файли використовують специфічні бібліотеки,
# їх потрібно додати до target_link_libraries вище.
# У нашому випадку всі залежності (Core, Sql, Widgets) вже є.

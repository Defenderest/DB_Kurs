#include <QApplication> // Use QApplication for GUI applications
#include <QDebug>
#include "mainwindow.h" // Include the main window header

int main(int argc, char *argv[])
{
    // 1. Create the QApplication object (required for any GUI application)
    QApplication a(argc, argv);

    // Optional: Set application details (useful for settings, etc.)
    QApplication::setApplicationName("Bookstore");
    QApplication::setOrganizationName("YourCompany"); // Replace if applicable
    QApplication::setApplicationVersion("1.0");

    qInfo() << "Starting the Bookstore application...";

    // 2. Create the main window instance
    //    The database connection and initial setup happen inside MainWindow's constructor now.
    MainWindow w;

    // 3. Show the main window
    w.show(); // Make the window visible

    qInfo() << "Application window shown. Entering event loop...";

    // 4. Start the Qt event loop.
    //    The application will run until the user closes the main window or quit() is called.
    //    The return value of exec() indicates how the application terminated.
    return a.exec();
}

#include <QCoreApplication>
#include <QDebug>
#include <QElapsedTimer> // Для замера времени
#include "database.h"
#include <windows.h>

int main(int argc, char *argv[])
{
    // Set console code pages for correct output (optional, but good practice if needed)
    // You might not need these if your system/IDE handles UTF-8 correctly,
    // or if you are only outputting English characters.
    // SetConsoleCP(1251); // Input Code Page
    // SetConsoleOutputCP(1251); // Output Code Page
    // For purely English output, these are less critical.

    QCoreApplication a(argc, argv);

    qInfo() << "Starting the program to create schema and populate PostgreSQL DB...";

    // --- Connection Parameters ---
    QString dbHost = "127.127.126.49";
    int dbPort = 5432;
    QString dbName = "postgres"; // Or the name of your target DB
    QString dbUser = "postgres";
    QString dbPassword = ""; // Enter your password if required
    int recordsToGenerate = 20; // Number of records to generate
    // --------------------------------

    DatabaseManager dbManager;
    QElapsedTimer timer; // Create a timer

    // 1. Connect to the database
    if (!dbManager.connectToDatabase(dbHost, dbPort, dbName, dbUser, dbPassword)) {
        qCritical() << "Failed to establish connection to the database. The program is terminating.";
        return 1;
    }

    // 2. Create (or recreate) the schema tables
    qInfo() << "\n=== Stage 1: Creating the database schema ===";
    timer.start(); // Start the timer
    if (!dbManager.createSchemaTables()) {
        qCritical() << "Failed to create the database schema. Check the error log.";
        return 1; // Exit with error
    }
    qInfo() << "Schema creation took:" << timer.elapsed() << "ms";


    // 3. Populate tables with test data
    qInfo() << "\n=== Stage 2: Populating tables with test data (" << recordsToGenerate << " records) ===";
    timer.restart(); // Restart the timer
    if (!dbManager.populateTestData(recordsToGenerate)) {
        qCritical() << "Failed to populate tables with test data. Check the error log.";
        // Connection will be closed in the destructor
        return 1; // Exit with error
    }
    qInfo() << "Data population took:" << timer.elapsed() << "ms";


    qInfo() << "\n=== Stage 3: Outputting data from tables ===";
    timer.restart();
    if (!dbManager.printAllData()) {
        qWarning() << "Errors occurred during data output (see log above).";
        // Not considering this a fatal error for program termination
    }
    qInfo() << "Data output took:" << timer.elapsed() << "ms";

    qInfo() << "\nProgram finished successfully.";
    return 0;
}

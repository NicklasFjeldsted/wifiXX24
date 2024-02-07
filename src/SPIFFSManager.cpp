/**
 * @file SPIFFSManager.cpp
 * @brief Implementation of SPIFFSManager functions for SPIFFS file system operations.
 */

#include "SPIFFSManager.h"

/**
 * @brief Initializes the SPIFFS file system.
 * 
 * This function mounts the SPIFFS file system and prints a success message if the mount is successful.
 * If the mount fails, an error message is printed.
 */
void initSPIFFS()
{
    if (!SPIFFS.begin(true))
    {
        Serial.println("An error has occurred while mounting SPIFFS");
        return;
    }
    Serial.println("SPIFFS mounted successfully");
}

/**
 * @brief Reads the content of a file from the file system.
 * 
 * @param fs The file system object.
 * @param path The path to the file.
 * @return The content of the file as a String.
 */
String readFile(fs::FS &fs, const char *path)
{
    Serial.printf("Reading file: %s\r\n", path);

    File file = fs.open(path);
    if (!file || file.isDirectory())
    {
        Serial.println("- failed to open file for reading");
        return String();
    }

    String fileContent;
    while (file.available())
    {
        fileContent += char(file.read());
    }
    return fileContent;
}

/**
 * @brief Writes a message to a file.
 * 
 * This function opens the specified file in write mode and writes the provided message to it.
 * 
 * @param fs The file system object to use for file operations.
 * @param path The path of the file to write to.
 * @param message The message to write to the file.
 */
void writeFile(fs::FS &fs, const char *path, const char *message)
{
    Serial.printf("Writing file: %s\r\n", path);

    File file = fs.open(path, FILE_WRITE);
    if (!file)
    {
        Serial.println("- failed to open file for writing");
        return;
    }
    if (file.print(message))
    {
        Serial.println("- file written");
    }
    else
    {
        Serial.println("- write failed");
    }
}

// SPIFFSManager.h
#ifndef SPIFFSManager_h
#define SPIFFSManager_h

#include "FS.h"
#include "SPIFFS.h"

void initSPIFFS();
String readFile(fs::FS &fs, const char *path);
void writeFile(fs::FS &fs, const char *path, const char *message);

#endif

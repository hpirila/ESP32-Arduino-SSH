#include "storage.hpp"
#include <FS.h>
#include <LittleFS.h>
#include <SD.h>
#include <SPIFFS.h>

namespace storage {
File fileOpen(FileLocation fileLocation,
              std::string fileMode,
              bool fileCreate) {
  File file;
  const char* fileName = fileLocation.filePath.c_str();
  const char* openingMode = fileMode.c_str();

  switch (fileLocation.fsType) {
    case FSType::sd:
      file = SD.open(fileName, openingMode, fileCreate);
      break;
    case FSType::spiffs:
      file = SPIFFS.open(fileName, openingMode, fileCreate);
      break;
    case FSType::littlefs:
      file = LittleFS.open(fileName, openingMode, fileCreate);
      break;
    default:
      break;
  }
  return file;
}

FSType findFile(std::string fileName) {
  if (SD.begin()) {
    if (SD.exists(fileName.c_str())) {
      return FSType::sd;
    } else {
      SD.end();
    }
  }
  if (SPIFFS.begin()) {
    if (SPIFFS.exists(fileName.c_str())) {
      return FSType::spiffs;
    } else {
      SPIFFS.end();
    }
  }
  if (LittleFS.begin()) {
    if (LittleFS.exists(fileName.c_str())) {
      return FSType::spiffs;
    } else {
      LittleFS.end();
    }
  }
  return FSType::none;
}

bool fileRemove(FileLocation fileLocation) {
  if (fileLocation.fsType == FSType::spiffs) {
    if (SPIFFS.remove(fileLocation.filePath.c_str())) {
      return true;
    }
  } else if (fileLocation.fsType == FSType::littlefs) {
    if (LittleFS.remove(fileLocation.filePath.c_str())) {
      return true;
    }
  } else if (fileLocation.fsType == FSType::sd) {
    if (SD.remove(fileLocation.filePath.c_str())) {
      return true;
    }
  }
  return false;
}

bool fileExists(FileLocation fileLocation) {
  if (fileLocation.fsType == FSType::spiffs) {
    if (SPIFFS.exists(fileLocation.filePath.c_str())) {
      return true;
    }
  } else if (fileLocation.fsType == FSType::littlefs) {
    if (LittleFS.exists(fileLocation.filePath.c_str())) {
      return true;
    }
  } else if (fileLocation.fsType == FSType::sd) {
    if (SD.exists(fileLocation.filePath.c_str())) {
      return true;
    }
  }
  return false;
}

long fileSystemTotalBytes(FSType fsType) {
  long totalBytes = 0;

  if (fsType == FSType::spiffs) {
    totalBytes = SPIFFS.totalBytes();
  } else if (fsType == FSType::littlefs) {
    totalBytes = LittleFS.totalBytes();
  } else if (fsType == FSType::sd) {
    totalBytes = SD.totalBytes();
  }
  return totalBytes;
}

long fileSystemUsedBytes(FSType fsType) {
  long usedBytes = 0;

  if (fsType == FSType::spiffs) {
    usedBytes = SPIFFS.usedBytes();
  } else if (fsType == FSType::littlefs) {
    usedBytes = LittleFS.usedBytes();
  } else if (fsType == FSType::sd) {
    usedBytes = SD.usedBytes();
  }
  return usedBytes;
}

bool fileSystemBegin(FSType fsType) {
  if (fsType == FSType::spiffs) {
    SPIFFS.begin(true);
    SPIFFS.end();
    if (SPIFFS.begin()) {
      return true;
    }
  } else if (fsType == FSType::littlefs) {
    if (LittleFS.begin()) {
      return true;
    }
  } else if (fsType == FSType::sd) {
    SD.begin();
    SD.end();
    if (SD.begin()) {
      return true;
    }
  }
  //  printE("File system begin failed");
  //  printlnE(fsType);

  return false;
}

void fileSystemEnd(FSType fsType) {
  if (fsType == FSType::spiffs) {
    SPIFFS.end();
  } else if (fsType == FSType::littlefs) {
    LittleFS.end();
  } else if (fsType == FSType::sd) {
    SD.end();
  }
}

FileLocation parseFsType(std::string fileName) {
  FileLocation fl;
  fl.fsType = FSType::none;
  fl.filePath = fileName;

  if (fileName.rfind("/spiffs/", 0) == 0) {
    fl.fsType = FSType::spiffs;
    fl.filePath = fileName.replace(0, 7, EMPTY_STRING);
    //    printlnI("FS is SPIFFS");
  } else if (fileName.rfind("/sd/", 0) == 0) {
    fl.fsType = FSType::sd;
    fl.filePath = fileName.replace(0, 3, EMPTY_STRING);
    //   printlnI("FS is SD");
  } else if (fileName.rfind("/littlefs/", 0) == 0) {
    fl.fsType = FSType::littlefs;
    fl.filePath = fileName.replace(0, 9, EMPTY_STRING);
    //   printlnI("FS is LITTLEFS");
  }
  // printI("File name is: ");
  // printlnI(fl.filePath.c_str());
  // debug("Exit");
  return fl;
}

}  // namespace storage

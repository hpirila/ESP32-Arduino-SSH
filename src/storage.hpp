#ifndef storage_hpp
#define storage_hpp

#include <Arduino.h>
#include <FS.h>

#define EMPTY_STRING ""

namespace storage {

enum FSType { none = 0, spiffs = 1, littlefs = 2, sd = 3 };
struct FileLocation {
  FSType fsType;
  std::string filePath;
};

File fileOpen(FileLocation fileLocation, std::string fileMode, bool fileCreate) ;
FSType findFile(std::string fileName) ;
bool fileRemove(FileLocation fileLocation);
bool fileExists(FileLocation fileLocation);
long fileSystemTotalBytes(FSType fsType);
long fileSystemUsedBytes(FSType fsType);
bool fileSystemBegin(FSType fsType);
void fileSystemEnd(FSType fsType);
FileLocation parseFsType(std::string fileName);

}  // namespace storage

#endif

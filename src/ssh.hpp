#ifndef SSH_hpp
#define SSH_hpp

#include <Arduino.h>
#include <libssh/libssh.h>

#define BUFFER_SIZE 4096

class SSH {
 private:
  ssh_session session;
  ssh_channel channel;
  bool init();
  bool connect(std::string host, std::string user);
  bool authenticateKey(std::string publicKey,
                       std::string privateKey,
                       const char* passPhrase);
  bool authenticatePassword(std::string password);

 public:
  bool isConnected;
  SSH();
  SSH(std::string hostIn,
      std::string userIn,
      std::string privateKeyIn,
      std::string publicKeyIn);
  SSH(std::string hostIn,
      std::string userIn,
      std::string privateKeyIn,
      std::string publicKeyIn,
      std::string passPhraseIn);
  SSH(std::string host, std::string user, std::string password);
  ~SSH();
  void connectWithPassword(std::string hostIn,
                           std::string userIn,
                           std::string passwordIn);
  void connectWithKey(std::string hostIn,
                      std::string userIn,
                      std::string privateKeyIn,
                      std::string publicKeyIn);
  void connectWithKey(std::string hostIn,
                      std::string userIn,
                      std::string privateKeyIn,
                      std::string publicKeyIn,
                      std::string passPhraseIn);
  bool sendCommand(std::string cmd);
  bool scpGetFile(std::string sourceFile, std::string destinationFile);
  bool scpPutFile(std::string sourceFile, std::string destinationFile);
  void end();
};

#endif

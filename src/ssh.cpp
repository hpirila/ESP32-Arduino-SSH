#include "ssh.hpp"
#include "libssh_esp32.h"
#include "storage.hpp"

SSH::SSH() {
  isConnected = false;
}

SSH::SSH(std::string host,
         std::string user,
         std::string publicKey,
         std::string privateKey) {
  connectWithKey(host, user, publicKey, privateKey);
}

SSH::SSH(std::string host,
         std::string user,
         std::string publicKey,
         std::string privateKey,
         std::string passPhrase) {
  connectWithKey(host, user, publicKey, privateKey, passPhrase);
}

SSH::SSH(std::string host, std::string user, std::string password) {
  connectWithPassword(host, user, password);
}

SSH::~SSH() {
  end();
}

void SSH::connectWithPassword(std::string host,
                              std::string user,
                              std::string password) {
  if (init()) {
    if (connect(host, user)) {
      if (authenticatePassword(password)) {
        isConnected = true;
      }
    }
  }
}

void SSH::connectWithKey(std::string host,
                         std::string user,
                         std::string publicKey,
                         std::string privateKey) {
  isConnected = false;
  if (init()) {
    if (connect(host, user)) {
      if (authenticateKey(publicKey, privateKey, NULL)) {
        isConnected = true;
      }
    }
  }
}

void SSH::connectWithKey(std::string host,
                         std::string user,
                         std::string publicKey,
                         std::string privateKey,
                         std::string passPhrase) {
  isConnected = false;
  if (init()) {
    if (connect(host, user)) {
      if (authenticateKey(publicKey, privateKey,
                          passPhrase.empty() ? NULL : passPhrase.c_str())) {
        isConnected = true;
      }
    }
  }
}

void SSH::end() {
  ssh_disconnect(session);
  ssh_free(session);
  ssh_finalize();
  isConnected = false;
}

bool SSH::init() {
  libssh_begin();
  ssh_init();
  session = ssh_new();
  if (session == NULL) {
    return false;
  }
  return true;
}

bool SSH::connect(std::string host, std::string user) {
  ssh_options_set(session, SSH_OPTIONS_HOST, host.c_str());
  ssh_options_set(session, SSH_OPTIONS_USER, user.c_str());
  ssh_options_set(session, SSH_OPTIONS_LOG_VERBOSITY, 0);

  int rc = ssh_connect(session);
  if (rc != SSH_OK) {
    return false;
  }
  return true;
}

bool SSH::authenticateKey(std::string publicKey,
                          std::string privateKey,
                          const char* passPhrase) {
  ssh_key key = NULL;
  int rc;

  storage::FileLocation publicKeyLocation;
  storage::FileLocation privateKeyLocation;

  publicKeyLocation = storage::parseFsType(publicKey);
  privateKeyLocation = storage::parseFsType(privateKey);

  if (!storage::fileSystemBegin(publicKeyLocation.fsType)) {
    return false;
  }

  rc = ssh_pki_import_pubkey_file(publicKey.c_str(), &key);

  if (rc != SSH_OK) {
    return false;
  }

  rc = ssh_userauth_try_publickey(session, NULL, key);
  if (rc != SSH_AUTH_SUCCESS) {
    return false;
  }

  ssh_key_free(key);
  storage::fileSystemEnd(publicKeyLocation.fsType);
  storage::fileSystemBegin(privateKeyLocation.fsType);

  rc = ssh_pki_import_privkey_file(privateKey.c_str(), passPhrase, NULL, NULL,
                                   &key);

  if (rc != SSH_AUTH_SUCCESS) {
    return false;
  }

  rc = ssh_userauth_publickey(session, NULL, key);
  if (rc != SSH_AUTH_SUCCESS) {
    return false;
  }

  ssh_key_free(key);
  fileSystemEnd(privateKeyLocation.fsType);

  return true;
}

bool SSH::authenticatePassword(std::string password) {
  int rc = ssh_userauth_password(session, NULL, password.c_str());
  if (rc != SSH_AUTH_SUCCESS) {
    return false;
  }

  return true;
}

bool SSH::sendCommand(std::string cmd) {
  ssh_channel channel;
  channel = ssh_channel_new(session);

  if (channel == NULL) {
    return false;
  }

  int rc = ssh_channel_open_session(channel);
  if (rc < 0) {
    return false;
  }

  ssh_channel_request_exec(channel, cmd.c_str());
  ssh_channel_close(channel);
  ssh_channel_free(channel);

  return true;
}

bool SSH::scpGetFile(std::string sourceFile, std::string destinationFile) {
  int bytesRead;
  long fileSize;
  long freeSpace;
  int resultCode;
  u_int8_t buffer[BUFFER_SIZE];
  storage::FileLocation fl;
  File file;

  fl = storage::parseFsType(destinationFile);

  if (!storage::fileSystemBegin(fl.fsType)) {
    return false;
  }

  ssh_scp scp = ssh_scp_new(session, SSH_SCP_READ, sourceFile.c_str());
  resultCode = ssh_scp_init(scp);
  if (resultCode != SSH_OK) {
    storage::fileSystemEnd(fl.fsType);
    ssh_scp_free(scp);
    return false;
  }

  resultCode = ssh_scp_pull_request(scp);
  if (resultCode != SSH_SCP_REQUEST_NEWFILE) {
    storage::fileSystemEnd(fl.fsType);
    ssh_scp_free(scp);
    return false;
  }

  fileSize = ssh_scp_request_get_size(scp);
  freeSpace = storage::fileSystemTotalBytes(fl.fsType) -
              storage::fileSystemUsedBytes(fl.fsType);

  if (fileSize > (freeSpace - std::numeric_limits<u_int8_t>::max())) {
    storage::fileSystemEnd(fl.fsType);
    ssh_scp_free(scp);
    return false;
  }

  if (storage::fileExists(fl)) {
    storage::fileRemove(fl);
  }

  file = storage::fileOpen(fl, "wb", true);
  if (!file) {
    storage::fileSystemEnd(fl.fsType);
    ssh_scp_free(scp);
    return false;
  }

  ssh_scp_accept_request(scp);

  long counter = 0;
  long total = 0;
  while (true) {
    bytesRead = ssh_scp_read(scp, buffer, BUFFER_SIZE);
    total += bytesRead;
    counter++;

    if (bytesRead < 0) {
      break;
    }
    for (int i = 0; i < bytesRead; i++) {
      file.write(buffer[i]);
    }
  }

  file.close();
  storage::fileSystemEnd(fl.fsType);
  ssh_scp_free(scp);

  return true;
}

bool SSH::scpPutFile(std::string sourceFile, std::string destinationFile) {
  int bytesRead;
  long fileSize;
  int resultCode;
  char buffer[BUFFER_SIZE];
  storage::FileLocation fl;
  File file;

  fl = storage::parseFsType(sourceFile);

  if (!storage::fileSystemBegin(fl.fsType)) {
    return false;
  }

  file = storage::fileOpen(fl, "r", false);
  if (!file) {
    return false;
  }

  fileSize = file.size();
  ssh_scp scp = ssh_scp_new(session, SSH_SCP_WRITE, destinationFile.c_str());
  resultCode = ssh_scp_init(scp);

  if (resultCode != SSH_OK) {
    file.close();
    storage::fileSystemEnd(fl.fsType);
    ssh_scp_free(scp);
    return false;
  }

  resultCode =
      ssh_scp_push_file(scp, sourceFile.c_str(), fileSize,
                        S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH);

  if (resultCode != SSH_OK) {
    file.close();
    storage::fileSystemEnd(fl.fsType);
    ssh_scp_free(scp);
    return false;
  }

  while (file.available()) {
    bytesRead = file.readBytes(buffer, BUFFER_SIZE);
    ssh_scp_write(scp, buffer, bytesRead);
  }

  file.close();
  storage::fileSystemEnd(fl.fsType);
  ssh_scp_free(scp);

  return true;
}

# ESP32 Arduino SSH wrapper class

This is a wrapper class and example program for [LibSSH-ESP32] Arduino SSH library.
This program features are:
- ssh connection to the server using 
    - Password authentication
    - Public key authentication
    - Public key authentication with passphrase encrypted private key
- execute a command over ssh connection (sshCommand)
- copy file from ESP32 to the server using scp (scpPut)
- copy file from the server to ESP32 using scp (scpGet)
- different file systems in ESP32
    - SPIFFS (path prefix is /spiffs/)
    - SD card (path prefix is /sd/)
    - LittleFS (path prefix is /littlefs/)

## Usage
You can compile this project in platformIO and upload it to ESP32. Modify main.cpp as per testing instructions below.

Alternatively, you can add this library to your platformIO project by adding these two dependencies in platformio.ini under lib_dep.

```
lib_deps = 
	ewpa/LibSSH-ESP32@^3.0.1
	https://github.com/hpirila/ESP32-Arduino-SSH.git
```

## Testing
Have a Linux server ready where you can ssh using a password or public key authentication. Password authentication may be easier to do first.
### Linux server
You need a Linux server where to access using ssh. You can create one, for example, to Google or Amazon cloud. This program follows an example where the Linux server is Ubuntu in the Amazon cloud, but any Linux distribution shall work.
To access the Linux server, you need to know its
- IP address or domain name
- Username and password
or
- Username and have the public and private key available for connection

### Configure Wifi
You need to set your WiFi SSID and password to this part in main.cpp.

```WiFi.begin("ssid", "Wifi_password");```

### Configure SSH connection for password authentication
#### In Linux server
By default, Ubuntu in Amazon AWS does not allow ssh using password authentication. You need to do three things to enable password authentication.

Set password for ubuntu user:

```sudo passwd ubuntu```

In this example, I set the password to ```System#1```

Enable ssh password authentication

```sudo nano /etc/ssh/sshd_config```

Change this line from

```PasswordAuthentication no```

to

```PasswordAuthentication yes```

Restart sshd process

```sudo systemctl restart sshd```

#### In main.cpp file

Set your Linux server domain name or IP address in this line. You can find the domain name in the Amazon AWS EC2 console, Instance summary, Public IPv4 DNS.

```ssh.connectWithPassword("ec2-111-112-113-114.ap-southeast-1.compute.amazonaws.com","ubuntu","System#1");```

That's all. You can now compile and upload the program to ESP32. 
- It should connect to the server using username ubuntu and password System#1. 
- create a file in the server called testFile1
- copy that file to the ESP32 SPIFFS file system
- copy the file back to server using different filename testFile2
- compares testFile1 and testFile2 and writes the result to result.txt

Login to the server and see if the files exist and the content of the result file.
```
ubuntu@my-server:~$ ls -l testFile* result.txt
-rw-rw-r-- 1 ubuntu ubuntu 44 Jan 22 10:25 result.txt
-rw-rw-r-- 1 ubuntu ubuntu 56 Jan 22 10:25 testFile1
-rw-rw-r-- 1 ubuntu ubuntu 56 Jan 22 10:25 testFile2
ubuntu@my-server:~$ cat result.txt
Files testFile1 and testFile2 are identical
ubuntu@my-server:~$
```

### Configure SSH connection for public key authentication
Now that we can copy files to ESP32, it is easy to configure public key authentication. We need first to generate the keys, add the public key to authorized_keys and copy the key files to ESP32
#### In Linux server
Generate key pair using ssh-keygen without a passphrase. Just press enter when it asks to enter a passphrase.
```
ubuntu@my-server:~$ ssh-keygen -f key1
Generating public/private rsa key pair.
Enter passphrase (empty for no passphrase):
Enter same passphrase again:
Your identification has been saved in key1
Your public key has been saved in key1.pub
The key fingerprint is:
SHA256:g21tszp4PlPBZp9ToOxgyWOog+HbahI8d5ABj7FbaAE ubuntu@my-server
The key's randomart image is:
+---[RSA 3072]----+
|E+.              |
|  B.        .    |
| = oo  o + . .   |
|. +o  .oB.B   .  |
|.o o...oS*+o o   |
|.oo.o. . ooo+    |
| .oo.. . ..  .   |
|. o . . =.       |
| o..   oo+       |
+----[SHA256]-----+
ubuntu@my-server:~$
```
And another key pair with a passphrase. Set passphrase to MyPassPhrase.
```
ubuntu@my-server:~$ ssh-keygen -f key2
Generating public/private rsa key pair.
Enter passphrase (empty for no passphrase):
Enter same passphrase again:
Your identification has been saved in key2
Your public key has been saved in key2.pub
The key fingerprint is:
SHA256:GL7UTforSMOQ2N1mSHmcdCM7NBPPnoc7jpsgvKPNVG4 ubuntu@my-server
The key's randomart image is:
+---[RSA 3072]----+
|       +Boo      |
|      o.+O .     |
|   o +.+o +      |
|  . +.o++* o     |
|     o=oS = .    |
|   . ++. . o     |
|    +.Eo  +      |
|   +.+...+ o     |
|  ..+.  +oo      |
+----[SHA256]-----+
```
Now add the two public keys to .ssh/authorized_keys file
```
cat key1.pub >> ~/.ssh/authorized_keys
cat key2.pub >> ~/.ssh/authorized_keys
chmod 600 ~/.ssh/authorized_keys
```
#### In main.cpp file
Uncomment these lines to copy the key files to ESP32
```
ssh.scpGetFile("key1", "/spiffs/key1");
ssh.scpGetFile("key1.pub", "/spiffs/key1.pub");
ssh.scpGetFile("key2", "/spiffs/key2");
ssh.scpGetFile("key2.pub", "/spiffs/key2.pub");
```
Upload the program to ESP32 and run it.

#### In Linux server
Now the key files are copied to ESP32, and we can remove the password from the ubuntu user and disable password authentication.

Remove password from user ubuntu

```sudo passwd -d ubuntu```

Disable ssh password authentication

```sudo nano /etc/ssh/sshd_config```

Change this line from

```PasswordAuthentication yes```

to

```PasswordAuthentication no```

Restart sshd process

```sudo systemctl restart sshd```

Remove files testFile1, testFile2 and result.txt

```rm ~/testFile1 ~/testFile2 ~/result.txt```

### Test public key authentication, no passphrase
#### In main.cpp file
Comment password authentication line and uncomment and edit public key authentication without passphrase line.
```
// ssh.connectWithPassword("ec2-111-112-113-114.ap-southeast-1.compute.amazonaws.com", "ubuntu","System#1");
ssh.connectWithKey("ec2-111-112-113-114.ap-southeast-1.compute.amazonaws.com", "ubuntu","/spiffs/key1.pub","/spiffs/key1");
```
Upload the program to ESP32 and run it. Check again on the Linux server that testFile1, testFile2 and result.txt are back with the correct content. Remove testFile1, testFile2 and result.txt again.
### Test public key authentication with a passphrase
#### In main.cpp file
Comment public key authentication without passphrase line and uncomment public key authentication with passphrase line
```
// ssh.connectWithKey("ec2-111-112-113-114.ap-southeast-1.compute.amazonaws.com", "ubuntu","/spiffs/key1.pub","/spiffs/key1");
ssh.connectWithKey("ec2-111-112-113-114.ap-southeast-1.compute.amazonaws.com", "ubuntu","/spiffs/key2.pub","/spiffs/key2","MyPassPhrase");
```
Upload the program to ESP32 and run it. Check again on the Linux server that testFile1, testFile2 and result.txt are back with the correct content.
## Troubleshooting
SPIFFS and LittleFS may not be formatted when you try the first time. You can see this error `E (3332) SPIFFS: mount failed`. Just press the ESP32 board reset button or re-upload to try again.

It can be helpful to download files from ESP32 back to the computer using Visual Studio Code and platformIO. It is possible using this [file system downloader plugin].


   [LibSSH-ESP32]: <https://github.com/ewpa/LibSSH-ESP32>
   [file system downloader plugin]: <https://github.com/maxgerhardt/pio-esp32-esp8266-filesystem-downloader>
   


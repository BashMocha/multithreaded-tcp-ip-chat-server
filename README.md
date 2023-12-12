# Classroom Chat Server
> Multithreaded server-client application where multiple clients can send text and files via server

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![Environment: Ubuntu](https://img.shields.io/badge/Environment-Ubuntu-orange)](https://ubuntu.com/)
 
<div align="center">
    <img src="https://github.com/CheesyFrappe/multithreaded-tcp-ip-chat-server/assets/80858788/4ec86731-aec1-4898-88a1-ba93e8fa4412"/>
</div><be>

---

### Table of contents
- [Introduction](#introduction)
- [Features](#features)
- [Prerequisities](#prerequisities)
- [Usage](#usage)

# Introduction
The project aims to help students to develop the idea of how client-server applications run and communicate. It's a simple implementation of server-client architecture that aims to simulate a classroom using TCP-IP protocol. 
Where students are able to join a server within the same port that the server uses, send texts to each other, ask/answer questions, and send/receive files. 

# Features
- The whole implementation runs under TCP/IP protocol.
- Server supports multi-clients up to 30. This can be modified via `MAX_CLIENT_NUM` in `util.h`.
- Server supports database functionality with SQLite3. Check out the [Prerequisities](#prerequisities).
- Server takes time for timeout. During the inactivity of 10 minutes, the server shuts itself. This can be modified via `TIMEOUT` in `util.h`.
- The packet size is 1 kilobyte. This can be modified via `BUFF_SIZE` in `util.h`.
- Each client supports multithreading for receiving and sending packets simultaneously.
- Students can send texts to each other through the server. This can also be mentioned as broadcasting.
- Students can ask, answer, and list all questions with defined commands. Check out the [Usage](#usage).
- Students can transfer and list files through the server with defined commands. Check out the [Usage](#usage).
- Students can list all current users on the server. The server keeps a list of active users in the database.

# Prerequisities
> [!IMPORTANT]
> The project was written/tested on Ubuntu/Linux. Some of the libraries are only compatible with Unix-like
> operating systems. Therefore, it does not run on Windows.

### SQLite3
To install from the command line:
```shell
  sudo apt install sqlite3
```
To verify the installation, check the software’s version:
```shell
  sqlite3 --version
```
Or you can check the official [Downloads](https://www.sqlite.org/download.html) page to install the source code.

# Usage
> [!WARNING]
> `./bin`, `./bin/local` and `./bin/remote` folders are crucial for the project functionality
> especially for the database and file transfer functions.
> Do not modify, edit, or delete `bin` and its sub-folders.

To install and build the project:
```shell
  git clone https://github.com/CheesyFrappe/multithreaded-tcp-ip-chat-server.git
  cd multithreaded-tcp-ip-chat-server/src
  make
```
Run server on bin directory:
```shell
  cd bin
  ./server
```
Run client(s) on bin directory:
```shell
  cd bin
  ./client
```
<br>
There are several defined commands for the utilities. Each of the commands is described below:

- use `help` to list all command descriptions during the session.
- use `quit` to exit from the application.
- use `ASK` to ask a question.
```shell
  ASK who is the first king of Portugal?  
```
- use `ANSWER` to answer a question.
```shell
  ANSWER it's Afonso I  
```
- use `LISTQUESTIONS` to list all questions.
```shell
  LISTQUESTIONS

  (1) who is the first king of Portugal?
      Afonso I

  ENDQUESTIONS 
```
- use `PUTFILE` to upload a file to the server from `bin/local` folder.
```shell
  PUTFILE 1.jpeg
  [+]Uploaded ./local/1.jpeg 111817  
```
- use `GETFILE` to download a file from the server to `bin/local` folder.
```shell
  GETFILE 1.jpeg
  [+]FILE ./local/1.jpeg 111817  
```
- use `LISTFILES` to list all files held by the server.
```shell
  LISTFILES

  (1) 1.jpeg

  (2) operating_systems.pdf

  ENDFILES
```
- use `LISTUSERS` to list all active users on the server.
```shell
  LISTUSERS

  (1) emir

  (2) miguel

  ENDUSERS 
```
<br>



#include <chrono>
#include <fineftp/server.h>

#include <string>
#include <thread>
#include <iostream>

int main()
{

#ifdef WIN32
  const std::string local_root = "D:\STORAGE"; // The backslash at the end is necessary!
#else                                          // WIN32
  const std::string local_root = "/";
#endif                                         // WIN32

  fineftp::FtpServer server(21);

  server.addUser("pacs", "pacs", local_root, fineftp::Permission::All);

  server.setCommandCallback([](const std::string &command, const std::string &param,
                               const fineftp::FtpReplyCode &reply_code, const std::string &reply_message)
                            {
    std::cout << "[CALLBACK] Command: " << command << " | Param: " << param
              << " | Reply: " << static_cast<int>(reply_code) << " | Msg: " << reply_message << std::endl;

  });
  // Start the FTP server with 4 threads. More threads will increase the
  // performance with multiple clients, but don't over-do it.
  server.start(4);

  // Prevent the application from exiting immediatelly
  for (;;)
  {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }

  return 0;
}

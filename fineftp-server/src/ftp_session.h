#pragma once

#include <asio.hpp> // IWYU pragma: keep

#include <deque>
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "ftp_message.h"
#include <fineftp/callback_types.h>

#include "filesystem.h"
#include "user_database.h"
#include "ftp_user.h"

#ifdef WIN32
#include "win_str_convert.h"
#endif // WIN32

namespace fineftp
{
  class ReadableFile;
  class WriteableFile;

  class FtpSession
      : public std::enable_shared_from_this<FtpSession>
  {
  private:
    ////////////////////////////////////////////////////////
    // Public API
    ////////////////////////////////////////////////////////
  public:
    FtpSession(asio::io_context &io_context, const UserDatabase &user_database, const std::function<void()> &completion_handler, std::ostream &output, std::ostream &error);

    // Copy (disabled, as we are inheriting from shared_from_this)
    FtpSession(const FtpSession &) = delete;
    FtpSession &operator=(const FtpSession &) = delete;

    // Move (disabled, as we are inheriting from shared_from_this)
    FtpSession &operator=(FtpSession &&) = delete;
    FtpSession(FtpSession &&) = delete;

    ~FtpSession();

    void start();

    asio::ip::tcp::socket &getSocket();

    void setCommandCallback(const FtpCommandCallback &callback) { command_callback_ = callback; }

    ////////////////////////////////////////////////////////
    // FTP command-socket
    ////////////////////////////////////////////////////////
  private:
    void sendFtpMessage(const FtpMessage &message);
    void sendFtpMessage(FtpReplyCode code, const std::string &message);
    void sendRawFtpMessage(const std::string &raw_message);
    void startSendingMessages();
    void readFtpCommand();

    void handleFtpCommand(const std::string &command);

    ////////////////////////////////////////////////////////
    // FTP Commands
    ////////////////////////////////////////////////////////
  private:
    // Access control commands
    void handleFtpCommandUSER(const std::string &param);
    void handleFtpCommandPASS(const std::string &param);
    void handleFtpCommandACCT(const std::string &param);
    void handleFtpCommandCWD(const std::string &param);
    void handleFtpCommandCDUP(const std::string &param);
    void handleFtpCommandREIN(const std::string &param);
    void handleFtpCommandQUIT(const std::string &param);

    // Transfer parameter commands
    void handleFtpCommandPORT(const std::string &param);
    void handleFtpCommandPASV(const std::string &param);
    void handleFtpCommandTYPE(const std::string &param);
    void handleFtpCommandSTRU(const std::string &param);
    void handleFtpCommandMODE(const std::string &param);

    // Ftp service commands
    void handleFtpCommandRETR(const std::string &param);
    void handleFtpCommandSIZE(const std::string &param);
    void handleFtpCommandSTOR(const std::string &param);
    void handleFtpCommandSTOU(const std::string &param);
    void handleFtpCommandAPPE(const std::string &param);
    void handleFtpCommandALLO(const std::string &param);
    void handleFtpCommandREST(const std::string &param);
    void handleFtpCommandRNFR(const std::string &param);
    void handleFtpCommandRNTO(const std::string &param);
    void handleFtpCommandABOR(const std::string &param);
    void handleFtpCommandDELE(const std::string &param);
    void handleFtpCommandRMD(const std::string &param);
    void handleFtpCommandMKD(const std::string &param);
    void handleFtpCommandPWD(const std::string &param);
    void handleFtpCommandLIST(const std::string &param);
    void handleFtpCommandNLST(const std::string &param);
    void handleFtpCommandSITE(const std::string &param);
    void handleFtpCommandSYST(const std::string &param);
    void handleFtpCommandSTAT(const std::string &param);
    void handleFtpCommandHELP(const std::string &param);
    void handleFtpCommandNOOP(const std::string &param);

    // Modern FTP Commands
    void handleFtpCommandFEAT(const std::string &param);

    void handleFtpCommandOPTS(const std::string &param);

    ////////////////////////////////////////////////////////
    // FTP data-socket send
    ////////////////////////////////////////////////////////
  private:
    void sendDirectoryListing(const std::map<std::string, Filesystem::FileStatus> &directory_content);
    void sendNameList(const std::map<std::string, Filesystem::FileStatus> &directory_content);

    void sendFile(const std::shared_ptr<ReadableFile> &file);

    void addDataToBufferAndSend(const std::shared_ptr<std::vector<char>> &data, const std::shared_ptr<asio::ip::tcp::socket> &data_socket);

    void writeDataToSocket(const std::shared_ptr<asio::ip::tcp::socket> &data_socket);

    ////////////////////////////////////////////////////////
    // FTP data-socket receive
    ////////////////////////////////////////////////////////
  private:
    void receiveFile(const std::shared_ptr<WriteableFile> &file);

    void receiveDataFromSocketAndWriteToFile(const std::shared_ptr<WriteableFile> &file, const std::shared_ptr<asio::ip::tcp::socket> &data_socket);

    void writeDataToFile(const std::shared_ptr<std::vector<char>> &data, const std::shared_ptr<WriteableFile> &file, const std::function<void(void)> &fetch_more = []()
                                                                                                                     { return; });

    void endDataReceiving(const std::shared_ptr<WriteableFile> &file, const std::shared_ptr<asio::ip::tcp::socket> &data_socket);

    ////////////////////////////////////////////////////////
    // Helpers
    ////////////////////////////////////////////////////////
  private:
    std::string toAbsoluteFtpPath(const std::string &rel_or_abs_ftp_path) const;
    std::string toLocalPath(const std::string &ftp_path) const;
    static std::string createQuotedFtpPath(const std::string &unquoted_ftp_path);

    /** @brief Checks if a path is renamable
     *
     * Checks if the current user can rename the given path. A path is renameable
     * if it exists, a user is logged in and the user has sufficient permissions
     * (file / dir / both) to rename it.
     *
     * @param ftp_path: The source path
     *
     * @return (COMMAND_OK, "") if the path can be renamed or any other meaningfull error message if not.
     */
    FtpMessage checkIfPathIsRenamable(const std::string &ftp_path) const;

    FtpMessage executeCWD(const std::string &param);

#ifdef WIN32
    /**
     * @brief Returns Windows' GetLastError() as human readable string
     * @return The message of the last error
     */
    static std::string GetLastErrorStr();
#endif // WIN32

    ////////////////////////////////////////////////////////
    // Member variables
    ////////////////////////////////////////////////////////
  private:
    // Completion handler
    const std::function<void()> completion_handler_;

    // User management
    const UserDatabase &user_database_;
    std::shared_ptr<FtpUser> logged_in_user_;

    // "Global" io service
    asio::io_context &io_context_;

    // Command Socket.
    // Note that the command_strand_ is used to serialize access to all of the 9 member variables following it.
    asio::io_context::strand command_strand_;
    asio::ip::tcp::socket command_socket_;
    asio::streambuf command_input_stream_;
    std::deque<std::string> command_output_queue_;

    std::string last_command_;
    std::string rename_from_path_;
    std::string username_for_login_;
    bool data_type_binary_;
    bool shutdown_requested_; // Set to true when the client sends a QUIT command.

    // Current state
    std::string ftp_working_directory_;

    // Data Socket (=> passive mode)
    asio::ip::tcp::acceptor data_acceptor_;

    // Note that the data_socket_strand_ is used to serialize access to the 2 member variables following it.
    asio::io_context::strand data_socket_strand_;
    std::weak_ptr<asio::ip::tcp::socket> data_socket_weakptr_;
    std::deque<std::shared_ptr<std::vector<char>>> data_buffer_;

    asio::steady_timer timer_;

    std::ostream &output_; /* Normal output log */
    std::ostream &error_;  /* Error output log */

    FtpCommandCallback command_callback_;

    std::string last_param_;
  };
}

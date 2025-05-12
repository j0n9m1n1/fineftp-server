#pragma once
#include <functional>
#include <string>
//#include "ftp_message.h"
#include "../../src/ftp_message.h"

namespace fineftp
{
    using FtpCommandCallback = std::function<void(const std::string &, const std::string &, const FtpReplyCode &, const std::string &)>;
}
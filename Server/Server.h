#pragma once
#include <functional>

namespace Server
{
    enum class WhatHappened
    {
        LibraryLoadingError,
        AddressInitializationError,
        AddressBindingError,
        SocketCreationError,
        Ok,
        RequestHandlerIsNull,
        SocketInitializationFailed,
        RequestProcessingFatalError,
        RecvFailed,
        SendFailed,
        ConnectionClosed
    };

    using Handler = const std::function<std::string(const std::string_view)> &;

    std::string ToString(const WhatHappened what);

    WhatHappened Start(Handler onRequest);
}
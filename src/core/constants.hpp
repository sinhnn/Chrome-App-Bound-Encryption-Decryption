#pragma once

namespace Core {
    class Constants
    {
    public:

        // Must greater than 2
        static constexpr int kMessageType_DecryptAppBoundEncryptedKey = 6;
        static constexpr int kMessageType_AppBoundKey = 7;
        static constexpr int kMessageType_EncryptAppBoundKey = 8;
        static constexpr int kMessageType_AppBoundKeyEncryptedKey = 9;
    };
}
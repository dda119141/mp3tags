#ifndef _TAG_STRING_CONVERSION
#define _TAG_STRING_CONVERSION

#include <unicode.hpp>


namespace tagBase
{
    std::string utf8ToUtf16String(const std::string& utf8String)
    {
        using utf16_t = Unicode::UTF16;

        const auto inputLength = utf8String.length();

        assert (inputLength > 0);

        const unsigned char *srcBegin = reinterpret_cast<const unsigned char*>(utf8String.data());
        const unsigned char *srcEnd   = srcBegin + inputLength;
        const auto destLen = inputLength;
        std::string destStr;
        destStr.reserve(destLen);

        utf16_t *dstBegin = reinterpret_cast<utf16_t*>(destStr.data());
        utf16_t *dstEnd   = dstBegin + destLen;

        Unicode::ConversionResult result = Unicode::ConvertUTF8toUTF16(
                &srcBegin, srcEnd, &dstBegin, dstEnd, Unicode::lenientConversion);

        //        std::cout << "result: " << destStr << std::endl;
        return destStr;

    }

    template<typename type>
        std::string getW16StringFromLatin(std::string_view val)
        {
            type data;

            const auto len = val.length() * 2;

            for(auto i=0; i < len; i+=2){
                data.push_back(val[i/2]);
                data.push_back(0x0);
            }

            return std::string(&data[0], len);
        }

    template<typename type>
        type getW16FromLatin(std::string_view val)
        {
            type data;

            data.reserve(val.length());

            for(auto i=0; i < val.length(); i++)
                data.push_back(static_cast<unsigned char>(val[i]));

            return data;
        }

}; // tagBase namespace


#endif

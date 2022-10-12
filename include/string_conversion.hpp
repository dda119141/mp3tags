// Copyright(c) 2020-present, Moge & contributors.
// Email: dda119141@gmail.com
// Distributed under the MIT License (http://opensource.org/licenses/MIT)

#ifndef _TAG_STRING_CONVERSION
#define _TAG_STRING_CONVERSION

#include <unicode.hpp>

namespace tagBase
{
std::string utf8ToUtf16String(const std::string &utf8String)
{
  using utf16_t = Unicode::UTF16;

  const auto inputLength = utf8String.length();

  assert(inputLength > 0);

  const unsigned char *srcBegin =
      reinterpret_cast<const unsigned char *>(utf8String.data());
  const unsigned char *srcEnd = srcBegin + inputLength;
  const auto destLen = inputLength;
  std::string destStr;
  destStr.reserve(destLen);

  utf16_t *dstBegin = reinterpret_cast<utf16_t *>(destStr.data());
  utf16_t *dstEnd = dstBegin + destLen;

  // Unicode::ConversionResult result = Unicode::ConvertUTF8toUTF16(
  //        &srcBegin, srcEnd, &dstBegin, dstEnd, Unicode::lenientConversion);
  //        std::cout << "result: " << destStr << std::endl;
  Unicode::ConvertUTF8toUTF16(&srcBegin, srcEnd, &dstBegin, dstEnd,
                              Unicode::lenientConversion);

  return destStr;
}

template <typename type>
std::string getW16StringFromLatin(std::string_view val)
{
#ifndef __OLD

  std::string val_str = std::string(val);
  return (std::accumulate(
              val_str.begin() + 1, val_str.end(), val_str.substr(0, 1),
              [](std::string arg, char ber) { return arg + '\0' + ber; }) +
          '\0');
#else
  // e.g type = std::vector<char>
  type data;

  const uint32_t len = val.length() * 2;

  for (uint32_t i = 0; i < len; i += 2) {
    data.push_back(static_cast<unsigned char>(val[i / 2]));
    data.push_back(0x0);
  }

  return std::string(&data[0], len);
#endif
}

const std::string &swapW16String(std::string &val_str)
{

  if (std::distance(val_str.begin(), val_str.end()) % 2 != 0)
    val_str += '\0';

  auto it = val_str.begin();
  while (it < val_str.end()) {
    std::swap(*it, *(it + 1));
    it += 2;
  }

  return val_str;
}
}; // namespace tagBase

#endif

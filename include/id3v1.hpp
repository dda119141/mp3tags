// Copyright(c) 2015-present, Moge & contributors.
// Email: dda119141@gmail.com
// Distributed under the MIT License (http://opensource.org/licenses/MIT)

#ifndef ID3V1_BASE
#define ID3V1_BASE

#include "id3.hpp"
#include "logger.hpp"
#include "result.hpp"
#include <iostream>

using namespace id3;

namespace id3v1 {

constexpr uint32_t GetTagSize = 128;

struct tagReadWriter {
private:
  const std::string &FileName;
  uint32_t tagBegin = 0;
  uint32_t tagPayload = 0;
  uint32_t tagPayloadLength = 0;
  uint32_t tagHeaderLength = 3;
  std::optional<id3::buffer_t> buffer;

public:
  explicit tagReadWriter(const std::string &fileName)
      : FileName(fileName), buffer({}) {
    std::ifstream filRead(FileName, std::ios::binary | std::ios::ate);

    const unsigned int dataSize = filRead.tellg();
    tagBegin = dataSize - ::id3v1::GetTagSize;
    filRead.seekg(tagBegin);

    id3::buffer_t tagHeaderBuffer =
        std::make_shared<std::vector<uint8_t>>(tagHeaderLength, '0');
    filRead.read(reinterpret_cast<char *>(tagHeaderBuffer->data()),
                 tagHeaderLength);

    const auto stringExtracted =
        id3::ExtractString(tagHeaderBuffer, 0, tagHeaderLength);
    const auto ret = (stringExtracted == std::string("TAG"));
    if (!ret) {
      throw id3::id3_error("id3v1: no tag string available");
    }

    tagPayload = tagBegin + tagHeaderLength;
    if (::id3v1::GetTagSize < tagHeaderLength)
      throw id3::id3_error("id3v1: no tag string available");

    tagPayloadLength = ::id3v1::GetTagSize - tagHeaderLength;

    filRead.seekg(tagPayload);
    id3::buffer_t buffer1 =
        std::make_shared<std::vector<unsigned char>>(tagPayloadLength, '0');
    filRead.read(reinterpret_cast<char *>(buffer1->data()), tagPayloadLength);
    buffer = buffer1;
  }

  uint32_t GetTagPayload() const { return tagPayload; }

  std::optional<id3::buffer_t> GetBuffer() const { return buffer; }
};

const expected::Result<id3::buffer_t> GetBuffer(const std::string &FileName) {
  try {
    const tagReadWriter tagRW{FileName};

    return expected::makeValue<id3::buffer_t>(tagRW.GetBuffer().value());

  } catch (const id3::id3_error &e) {
    //  std::cout << e.what() << std::endl;
    return expected::makeError<id3::buffer_t>("id3v1 object not valid");
  }
}

class frameObj_t {
public:
  explicit frameObj_t(id3::buffer_t buf, uint32_t frameStartPosIn,
                      uint32_t frameEndPosIn)
      : tagBuffer(buf), frameStartPos(frameStartPosIn),
        frameEndPos(frameEndPosIn) {

    if (frameStartPos > frameEndPos)
      throw id3::id3_error("id3v1: no tag string available");

    frameContent = id3::ExtractString(tagBuffer, frameStartPos,
                                      (frameEndPos - frameStartPos));

    id3::stripLeft(frameContent.value());
  }

  id3::buffer_t tagBuffer;
  uint32_t frameStartPos = 0;
  uint32_t frameEndPos = 0;
  std::optional<std::string> frameContent = {};
};

const auto GetTheFramePayload(id3::buffer_t buffer, uint32_t start,
                              uint32_t end) {

  const frameObj_t frameObj{buffer, start, end};

  return frameObj.frameContent.value();
}

const std::optional<bool> SetFramePayload(const std::string &filename,
                                          std::string_view content,
                                          uint32_t relativeFramePayloadStart,
                                          uint32_t relativeFramePayloadEnd) {
  assert(relativeFramePayloadEnd > relativeFramePayloadStart);

  const tagReadWriter tagRW{filename};

  if (content.size() > (relativeFramePayloadEnd - relativeFramePayloadStart)) {
    ID3_LOG_ERROR("content length {} too big for frame area", content.size());

    return {};
  }

  frameScopeProperties frameScopeProperties = {};
  frameScopeProperties.frameIDStartPosition =
      tagRW.GetTagPayload() + relativeFramePayloadStart;
  frameScopeProperties.frameContentStartPosition =
      tagRW.GetTagPayload() + relativeFramePayloadStart;
  frameScopeProperties.frameLength =
      relativeFramePayloadEnd - relativeFramePayloadStart;

  ID3_LOG_INFO("ID3V1: Write content: {} at {}", std::string(content),
               tagRW.GetTagPayload());

  return WriteFile(filename, std::string(content), frameScopeProperties);
}

const std::optional<bool> SetTitle(const std::string &filename,
                                   std::string_view content) {
  try {
    return id3v1::SetFramePayload(filename, content, 0, 30);
  } catch (const std::exception &e) {
    std::cout << e.what() << std::endl;
    return {};
  }
}

const std::optional<bool> SetLeadArtist(const std::string &filename,
                                        std::string_view content) {
  try {
    return id3v1::SetFramePayload(filename, content, 30, 60);
  } catch (const std::exception &e) {
    std::cout << e.what() << std::endl;
    return {};
  }
}

const std::optional<bool> SetAlbum(const std::string &filename,
                                   std::string_view content) {
  try {
    return id3v1::SetFramePayload(filename, content, 60, 90);
  } catch (const std::exception &e) {
    std::cout << e.what() << std::endl;
    return {};
  }
}

const std::optional<bool> SetYear(const std::string &filename,
                                  std::string_view content) {
  try {
    return id3v1::SetFramePayload(filename, content, 90, 94);
  } catch (const std::exception &e) {
    std::cout << e.what() << std::endl;
    return {};
  }
}

const std::optional<bool> SetComment(const std::string &filename,
                                     std::string_view content) {
  try {
    return id3v1::SetFramePayload(filename, content, 94, 124);
  } catch (const std::exception &e) {
    std::cout << e.what() << std::endl;
    return {};
  }
}

const std::optional<bool> SetGenre(const std::string &filename,
                                   std::string_view content) {
  try {
    return id3v1::SetFramePayload(filename, content, 124, 125);
  } catch (const std::exception &e) {
    std::cout << e.what() << std::endl;
    return {};
  }
}

const auto GetTitle(const std::string &filename) {
  return id3v1::GetBuffer(filename) |
         [](id3::buffer_t buffer) { return GetTheFramePayload(buffer, 0, 30); };
}

const auto GetLeadArtist(const std::string &filename) {
  return id3v1::GetBuffer(filename) | [](id3::buffer_t buffer) {
    return GetTheFramePayload(buffer, 30, 60);
  };
}

const auto GetAlbum(const std::string &filename) {
  return id3v1::GetBuffer(filename) | [](id3::buffer_t buffer) {
    return GetTheFramePayload(buffer, 60, 90);
  };
}

const auto GetYear(const std::string &filename) {
  return id3v1::GetBuffer(filename) | [](id3::buffer_t buffer) {
    return GetTheFramePayload(buffer, 90, 94);
  };
}

const auto GetComment(const std::string &filename) {
  return id3v1::GetBuffer(filename) | [](id3::buffer_t buffer) {
    return GetTheFramePayload(buffer, 94, 124);
  };
}

const auto GetGenre(const std::string &filename) {
  const auto ret = id3v1::GetBuffer(filename) | [](id3::buffer_t buffer) {
    return GetTheFramePayload(buffer, 124, 125);
  };
  return ret;
}

}; // end namespace id3v1

#endif // ID3V1_BASE

// Copyright(c) 2020-present, Moge & contributors.
// Email: dda119141@gmail.com
// Distributed under the MIT License (http://opensource.org/licenses/MIT)

#ifndef _EXTEND_TAG_AREA
#define _EXTEND_TAG_AREA
#include "id3.hpp"
#include "id3_exceptions.hpp"
#include "id3v2_base.hpp"

namespace id3v2 {

class bufferExtended {
private:
  buffer_t tagBuffer;

public:
  explicit bufferExtended(const basicParameters &fileParameter,
                          uint32_t additionalSize) {
    const auto frameConfig = fileParameter.get_frame_settings();
    constexpr uint32_t tagsSizePositionInHeader = 6;
    constexpr uint32_t frameSizePositionInFrameHeader = 4;

    tagBuffer = CreateTagBufferFromFile(fileParameter.get_filename(),
                                        GetTagHeaderSize<uint32_t>());

    if (frameConfig.getFrameKeyOffset() + frameConfig.getFramePayloadLength() <
        tagBuffer->size()) {
      ID3V2_THROW("error : Frame Key Offset + Payload length > total tag size");
    }
    if (tagBuffer->size() < 10) {
      ID3V2_THROW("error : tagBuffer length < 10");
    }

    const auto tagSizeBuffer = updateTagSize(tagBuffer, additionalSize);

    const auto frameSizeBuff =
        updateFrameSizeIndex<buffer_t, uint32_t, uint32_t>(
            fileParameter.get_tag_version(), tagBuffer, additionalSize,
            frameConfig.getFrameKeyOffset());

    id3::replaceElementsInBuff(tagSizeBuffer.value(), tagBuffer,
                               tagsSizePositionInHeader);

    id3::replaceElementsInBuff(frameSizeBuff.value(), tagBuffer,
                               frameConfig.getFrameKeyOffset() +
                                   frameSizePositionInFrameHeader);

    const auto it = tagBuffer->begin() + frameConfig.getFramePayloadOffset() +
                    frameConfig.getFramePayloadLength();

    tagBuffer->insert(it, additionalSize, 0);
  }

  buffer_t get_tag_buffer() const { return tagBuffer; }
};

class FileExtended {
public:
  explicit FileExtended(const basicParameters &FileParameter,
                        uint32_t additionalSize, const std::string &content)
      : fileParameter(FileParameter) {
    std::call_once(m_once, [&content, additionalSize, this] {
      assert(fileParameter.get_frame_settings().getFramePayloadLength() >=
             content.size());

      const auto Buffer_obj = bufferExtended{fileParameter, additionalSize};

      const auto filled_buffer = fillTagBufferWithPayload<std::string_view>(
          content, Buffer_obj.get_tag_buffer(),
          fileParameter.get_frame_settings());

      this->ReWriteFile(filled_buffer, additionalSize);

      status = true;
    });
  }

  bool get_status() const { return status; }

private:
  const basicParameters &fileParameter;
  bool status = false;

  std::once_flag m_once;

  void ReWriteFile(id3::buffer_t tagBuffer, uint32_t extraSize) {

    std::ifstream filRead(fileParameter.get_filename(),
                          std::ios::binary | std::ios::ate);

    const auto _headerAndTagsSize = GetTotalTagSize(tagBuffer);
    const uint32_t tagsSizeOld = _headerAndTagsSize - extraSize;

    uint32_t fileSize = filRead.tellg();
    if (fileSize < tagsSizeOld) {
      ID3V2_THROW("error : file size > total old tag size");
    }
    if (fileSize < tagBuffer->size()) {
      ID3V2_THROW("error : file size < new tag size");
    }
    fileSize -= tagsSizeOld;

    /* go to the real audio payload position and read it
            into buffer bufRead */
    filRead.seekg(tagsSizeOld);
    auto bufRead = std::make_shared<std::vector<uint8_t>>();
    bufRead->reserve(fileSize);
    filRead.read(reinterpret_cast<char *>(&bufRead->at(0)), fileSize);

    /* create new file and write tag buffer */
    std::ofstream filWrite(fileParameter.get_filename() + modifiedEnding,
                           std::ios::binary | std::ios::app);

    std::for_each(std::begin(*tagBuffer), std::end(*tagBuffer),
                  [&filWrite](const char &n) { filWrite << n; });

    /* now write real audio payload*/
    std::for_each(bufRead->begin(), bufRead->begin() + fileSize,
                  [&filWrite](const char &n) { filWrite << n; });
  };
};

} // namespace id3v2

#endif

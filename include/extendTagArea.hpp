// Copyright(c) 2020-present, Moge & contributors.
// Email: dda119141@gmail.com
// Distributed under the MIT License (http://opensource.org/licenses/MIT)

#ifndef _EXTEND_TAG_AREA
#define _EXTEND_TAG_AREA
#include "id3.hpp"
#include "id3_exceptions.hpp"
#include "id3v2_base.hpp"

namespace id3v2 {

void ExtendTagBuffer(const AudioSettings_t *const audioProperties,
                     std::vector<char> &tagBuffer, uint32_t additionalSize) {

  const auto &frameProperties =
      audioProperties->frameScopePropertiesObj.value();

  tagBuffer.reserve(tagBuffer.size() + additionalSize);

  const auto tagSizeBuffer = updateTagSize(tagBuffer, additionalSize);

  const auto frameSizeBuff =
      updateFrameSizeIndex<std::vector<char>, uint32_t, uint32_t>(
          audioProperties->fileScopePropertiesObj.get_tag_version(), tagBuffer,
          additionalSize, frameProperties.frameIDStartPosition);

  id3::replaceElementsInBuff(tagSizeBuffer.value(), tagBuffer,
                             tagSizePositionInHeader);

  id3::replaceElementsInBuff(frameSizeBuff.value(), tagBuffer,
                             frameProperties.frameIDStartPosition +
                                 frameSizePositionInFrameHeader);

  const auto it = tagBuffer.begin() +
                  frameProperties.frameContentStartPosition +
                  frameProperties.getFramePayloadLength();

  tagBuffer.insert(it, additionalSize, 0);
}

class FileExtended {
public:
  explicit FileExtended(std::vector<char> &tagBufIn,
                        const audioProperties_t *const audioProperties,
                        uint32_t additionalSize, const std::string &content)
      : mTagBuffer(tagBufIn), audioPropertiesObj(audioProperties) {

    CheckAudioPropertiesObject(audioPropertiesObj);

    const auto &frameProperties =
        audioPropertiesObj->frameScopePropertiesObj.value();
    const auto &fileParameter = audioPropertiesObj->fileScopePropertiesObj;

    if (frameProperties.frameIDStartPosition +
            frameProperties.getFramePayloadLength() >
        mTagBuffer.size()) {
      ID3V2_THROW("error : Frame Key Offset + Payload length > total tag size");
    }
    if (mTagBuffer.size() < id3v2::TagHeaderSize) {
      ID3V2_THROW("error : tagBuffer length < TagHeaderSize");
    }

    ExtendTagBuffer(audioPropertiesObj, mTagBuffer, additionalSize);

    fillTagBufferWithPayload<std::string_view>(content, mTagBuffer,
                                               frameProperties);

    if (this->ReWriteFile(mTagBuffer, additionalSize)) {
      id3::renameFile(fileParameter.get_filename() + modifiedEnding,
                      fileParameter.get_filename());
    }
  }

  auto get_status() const { return status; }

private:
  std::vector<char> mTagBuffer;
  const audioProperties_t *const audioPropertiesObj;
  execution_status_t status{};

  bool ReWriteFile(const std::vector<char> &tagBuffer, uint32_t extraSize) {
    const auto &fileParameter = audioPropertiesObj->fileScopePropertiesObj;

    std::ifstream filRead(fileParameter.get_filename(),
                          std::ios::binary | std::ios::ate);

    const auto extendedTagSize = tagBuffer.size();
    const uint32_t initialTagSize = extendedTagSize - extraSize;

    uint32_t fileSize = filRead.tellg();
    if (fileSize < initialTagSize) {
      ID3V2_THROW("error : file size > total old tag size");
    }
    if (fileSize < tagBuffer.size()) {
      ID3V2_THROW("error : file size < new tag size");
    }

    /* go to the real audio payload position and read it
            into buffer bufRead */
    const auto audioSizeWithoutId3v2Tag = fileSize - initialTagSize;
    filRead.seekg(id3v2::TagHeaderStartPosition + initialTagSize);
    auto bufRead =
        std::make_unique<std::vector<char>>(audioSizeWithoutId3v2Tag);
    filRead.read(reinterpret_cast<char *>(&bufRead->at(0)),
                 audioSizeWithoutId3v2Tag);

    /* create new file and write tag buffer */
    std::ofstream filWrite(fileParameter.get_filename() + modifiedEnding,
                           std::ios::binary | std::ios::app);

    /* write tag buffer into file */
    std::for_each(std::begin(tagBuffer), std::end(tagBuffer),
                  [&filWrite](const char &n) { filWrite << n; });

    /* now write real audio payload into Buffer*/
    std::for_each(bufRead->begin(), bufRead->end(),
                  [&filWrite](const char &n) { filWrite << n; });
    return true;
  }
};

} // namespace id3v2

#endif

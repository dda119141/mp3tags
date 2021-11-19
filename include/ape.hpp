// Copyright(c) 2020-present, Moge & contributors.
// Email: dda119141@gmail.com
// Distributed under the MIT License (http://opensource.org/licenses/MIT)

#ifndef APE_BASE
#define APE_BASE

#include "id3.hpp"
#include "logger.hpp"
#include "result.hpp"

namespace ape {

static const std::string modifiedEnding(".ape.mod");

constexpr uint32_t GetTagFooterSize(void) { return static_cast<uint32_t>(32); }

constexpr uint32_t GetTagHeaderSize(void) { return static_cast<uint32_t>(32); }

constexpr uint32_t OffsetFromFrameStartToFrameID(void) { return static_cast<uint32_t>(8); }

typedef struct _frameConfig {
    uint32_t frameStartPosition;
    uint32_t frameContentPosition;
    uint32_t frameLength;
    std::string frameContent;
} frameProperties_t;

std::string extractTheTag(id3::buffer_t buffer,
                                                  uint32_t start,
                                                  uint32_t length) {
	auto tagArea = id3::ExtractString(buffer, start, length);

	auto tagAreaFinal = std::string(id3::stripLeft(tagArea));
	
	return tagAreaFinal;
}

std::optional<id3::buffer_t> UpdateFrameSize(id3::buffer_t buffer,
                                         uint32_t extraSize,
                                         uint32_t frameIDPosition) {
    const uint32_t frameSizePositionInArea = frameIDPosition;
    constexpr uint32_t frameSizeLengthInArea = 4;
    constexpr uint32_t frameSizeMaxValuePerElement = 255;

    id3::log()->info(" Frame Index: {}", frameIDPosition);
    id3::log()->info(
        "Tag Frame Bytes before update : {}",
        spdlog::to_hex(std::begin(*buffer) + frameSizePositionInArea,
                       std::begin(*buffer) + frameSizePositionInArea + 4));
    const auto ret =  id3::updateAreaSize<uint32_t>(
        buffer, extraSize, frameSizePositionInArea, frameSizeLengthInArea,
        frameSizeMaxValuePerElement, false);

    return ret;
}

class apeTagProperties {
private:
	std::once_flag m_once;
	const std::string& FileName;
	uint32_t mfileSize = 0;
	uint32_t mTagSize = 0;
	uint32_t mNumberOfFrames = 0;
	uint32_t mTagFooterBegin = 0;
	uint32_t mTagPayloadPosition = 0;
	uint32_t mTagStartPosition = 0;
	id3::buffer_t buffer = {};

	const uint32_t getTagSize(std::ifstream& fRead, uint32_t bufferLength) {
		auto tagLengthBuffer = std::make_shared<std::vector<unsigned char>>(bufferLength);
		fRead.read(reinterpret_cast<char*>(tagLengthBuffer->data()),
			bufferLength);

		return id3::GetTagSizeDefault(tagLengthBuffer, bufferLength);
	}

	const std::string readString(std::ifstream& fRead, uint32_t bufferLength) {

		id3::buffer_t Buffer = std::make_shared<std::vector<unsigned char>>(bufferLength);
		fRead.read(reinterpret_cast<char*>(Buffer->data()), bufferLength);

		return id3::ExtractString(Buffer, 0, bufferLength);

	}

	bool checkAPEtag(std::ifstream& fRead, uint32_t bufferLength,
		const std::string& tagToCheck) {
		assert(tagToCheck.size() <= bufferLength);
		const auto stringToCheck = readString(fRead, bufferLength);

		if (bool ret = (stringToCheck == tagToCheck);  !ret) {
			ID3_LOG_WARN("error: {} and {}", stringToCheck, tagToCheck);
			return ret;
		}
		else {
			return ret;
		}
	}

public:
	explicit apeTagProperties(const std::string& fileName,
		uint32_t id3v1TagSizeIfPresent)
		: FileName{ fileName }
	{
        std::call_once(m_once, [this, &fileName, &id3v1TagSizeIfPresent]() {
            uint32_t footerPreambleOffsetFromEndOfFile = 0;

            std::ifstream filRead(FileName, std::ios::binary | std::ios::ate);
           
			{
				if (id3v1TagSizeIfPresent)
					footerPreambleOffsetFromEndOfFile =
					id3v1::GetTagSize() + ape::GetTagFooterSize();
				else
					footerPreambleOffsetFromEndOfFile = ape::GetTagFooterSize();

				mfileSize = filRead.tellg();
				mTagFooterBegin = mfileSize - footerPreambleOffsetFromEndOfFile;
				filRead.seekg(mTagFooterBegin);
			}
            if (uint32_t APEpreambleLength = 8; !checkAPEtag(filRead, APEpreambleLength,
                                std::string("APETAGEX"))) {
				APE_THROW("APE apeTagProperties: APETAGEX is not there");
            }
            if (uint32_t mVersion = getTagSize(filRead, 4); (mVersion != 1000) && (mVersion != 2000)) {
				APE_THROW("APE apeTagProperties Version: Version = {}",
                                mVersion);
            }
            if (uint32_t mTagSize = getTagSize(filRead, 4); mTagSize == 0) {
				APE_THROW("APE apeTagProperties: tagsize = 0");
            }
            if (uint32_t mNumberOfFrames = getTagSize(filRead, 4); mNumberOfFrames == 0) {
				APE_THROW("APE apeTagProperties: No frame!");
            }

            mTagStartPosition = mTagFooterBegin - mTagSize;

            mTagPayloadPosition =
                mTagFooterBegin + GetTagFooterSize() - mTagSize;

            filRead.seekg(mTagStartPosition);
            buffer = std::make_shared<std::vector<unsigned char>>(
                mTagSize + GetTagFooterSize(), '0');
            filRead.read(reinterpret_cast<char*>(buffer->data()),
                            mTagSize + GetTagFooterSize());
        });
    }

    const uint32_t getTagPayloadPosition() const { return mTagPayloadPosition; }
    const uint32_t getTagStartPosition() const { return mTagStartPosition; }
    const uint32_t getTagFooterBegin() const { return mTagFooterBegin; }
    const uint32_t getFileLength() const { return mfileSize; }
    id3::buffer_t GetBuffer() const { return buffer; }
};

class tagReader {

public:
	explicit tagReader(const std::string& FileName, std::string_view FrameID)
		: filename{FileName}, frameID{ FrameID }
	{
		std::call_once(m_once, [this]() {
			bool bContinue = true;

			try {
				tagProperties = std::make_unique<apeTagProperties>(filename, true);				
			}
			catch (const id3::ape_error & e) {
				bContinue = false;
			}
			if(!bContinue){ // id3v1 not present
				try {
					tagProperties.reset(new apeTagProperties(filename, false));
				}
				catch (const id3::ape_error& e) {
					bContinue = false;
					APE_THROW("ape not valid!");
				}
			}			
			if (bContinue) {
				getFrameProperties();
			}
			});
	}

	const std::string getFramePayload(void) const
	{
		return frameProperties->frameContent;
	}

	const std::optional<bool> writeFramePayload(std::string_view framePayload,
		uint32_t start, uint32_t length) const 
	{
		if (framePayload.size() > length) {
			APE_THROW("framePayload length too big for frame area");
		}

		const auto frameGlobalConfig = FrameSettings::create()
			->with_frameID_offset(frameProperties->frameStartPosition + OffsetFromFrameStartToFrameID())
			.with_framecontent_offset(frameProperties->frameContentPosition)
			.with_frame_length(frameProperties->frameLength);

		if (frameProperties->frameLength >= framePayload.size()) {

			return WriteFile(filename, std::string(framePayload),
				frameGlobalConfig);
		}
		else {
			const uint32_t additionalSize = framePayload.size() - frameProperties->frameLength;

			const auto writeBackAction = extendTagBuffer(frameGlobalConfig, framePayload, additionalSize) |
				[&](id3::buffer_t buffer) {
				return this->ReWriteFile(buffer);
			};

			return writeBackAction | [&](bool fileWritten) {
				return id3::renameFile(filename + ape::modifiedEnding, filename);
			};
		}
	}

	/* function not thread safe */
	expected::Result<bool> ReWriteFile(id3::buffer_t buff) const 
	{
		const uint32_t endOfFooter = tagProperties->getTagFooterBegin() + GetTagFooterSize();

		std::ifstream filRead(filename, std::ios::binary | std::ios::ate);

		const uint32_t fileSize = filRead.tellg();
		assert(fileSize >= endOfFooter);
		const uint32_t endLength = fileSize - endOfFooter;

		/* read in bytes from footer end until audio file end */
		std::vector<uint8_t> bufFooter;
		bufFooter.reserve((endLength));
		filRead.seekg(endOfFooter);
		filRead.read(reinterpret_cast<char*>(&bufFooter[0]), endLength);

		/* read in bytes from audio file start end until ape area start */
		std::vector<uint8_t> bufHeader;
		bufHeader.reserve((tagProperties->getTagStartPosition()));
		filRead.seekg(0);
		filRead.read(reinterpret_cast<char*>(&bufHeader[0]),
			tagProperties->getTagStartPosition());

		filRead.close();

		const std::string writeFileName =
			filename + ::ape::modifiedEnding;
		ID3_LOG_INFO("file to write: {}", writeFileName);

		std::ofstream filWrite(writeFileName, std::ios::binary | std::ios::app);

		filWrite.seekp(0);
		std::for_each(std::begin(bufHeader),
			std::begin(bufHeader) + tagProperties->getTagStartPosition(),
			[&filWrite](const char& n) { filWrite << n; });

		filWrite.seekp(tagProperties->getTagStartPosition());
		std::for_each(std::begin(*buff), std::end(*buff),
			[&filWrite](const char& n) { filWrite << n; });

		filWrite.seekp(endOfFooter);
		std::for_each(bufFooter.begin(), bufFooter.begin() + endLength,
			[&filWrite](const char& n) { filWrite << n; });

		return expected::makeValue<bool>(true);
	}

	const expected::Result<id3::buffer_t> extendTagBuffer(
		const id3::FrameSettings& frameConfig, std::string_view framePayload, uint32_t additionalSize) const {

		const uint32_t relativeBufferPosition = tagProperties->getTagStartPosition();
		const uint32_t tagsSizePositionInHeader =
			tagProperties->getTagStartPosition() - relativeBufferPosition + 12;
		const uint32_t tagsSizePositionInFooter =
			tagProperties->getTagFooterBegin() - relativeBufferPosition + 12;
		const uint32_t frameSizePositionInFrameHeader =
			frameConfig.getFrameKeyOffset() - OffsetFromFrameStartToFrameID() - relativeBufferPosition;
		const uint32_t frameContentStart =
			frameConfig.getFramePayloadOffset() - relativeBufferPosition;

		auto cBuffer = tagProperties->GetBuffer();

		const auto tagSizeBuff =
			UpdateFrameSize(cBuffer, additionalSize, tagsSizePositionInHeader);

		const auto frameSizeBuff = UpdateFrameSize(cBuffer, additionalSize,
			frameSizePositionInFrameHeader);

		assert(frameConfig.getFramePayloadLength() <= cBuffer->size());

		auto finalBuffer = std::move(cBuffer);

		id3::replaceElementsInBuff(frameSizeBuff.value(), finalBuffer,
			frameSizePositionInFrameHeader);

		id3::replaceElementsInBuff(tagSizeBuff.value(), finalBuffer,
			tagsSizePositionInHeader);

		id3::replaceElementsInBuff(tagSizeBuff.value(), finalBuffer,
			tagsSizePositionInFooter);

		auto it = finalBuffer->begin() + frameContentStart +
			frameConfig.getFramePayloadLength();
		finalBuffer->insert(it, additionalSize, 0);

		const auto iter = std::begin(*finalBuffer) + frameContentStart;
		std::transform(iter, iter + framePayload.size(), framePayload.begin(), iter,
			[](char a, char b) { return b; });

		return expected::makeValue<id3::buffer_t>(finalBuffer);
	}

private:
    std::once_flag m_once;
    const std::string& filename;
    std::string_view frameID;
    std::unique_ptr<apeTagProperties> tagProperties;
	std::unique_ptr<frameProperties_t> frameProperties = {};

	void getFrameProperties()
	{
		auto buffer = tagProperties->GetBuffer();

		const auto tagArea = id3::ExtractString(buffer, 0, buffer->size());

		constexpr uint32_t frameKeyTerminatorLength = 1;
		auto searchFramePosition = id3::searchFrame<std::string_view>(tagArea);
		const auto frameIDPosition = searchFramePosition(frameID);
		const uint32_t frameContentPosition =
			frameIDPosition + frameID.size() + frameKeyTerminatorLength;

		if (!frameContentPosition) {
			APE_THROW("frameContentPosition = 0");
		}
		if (frameIDPosition < OffsetFromFrameStartToFrameID()) {
			APE_THROW("Frame ID position < OffsetFromFrameStartToFrameID")
		}

		const uint32_t frameStartPosition =
			frameIDPosition - OffsetFromFrameStartToFrameID();
		const uint32_t frameLength =
			id3::GetTagSizeDefault(buffer, 4, frameStartPosition);

		frameProperties = std::make_unique<frameProperties_t>();
		frameProperties->frameContentPosition = frameContentPosition + tagProperties->getTagStartPosition();
		frameProperties->frameStartPosition = frameStartPosition + tagProperties->getTagStartPosition();
		frameProperties->frameLength = frameLength;

		frameProperties->frameContent = extractTheTag(buffer, frameContentPosition, frameLength);

	}
};  // class tagReader


const expected::Result<std::string> getFramePayload(const std::string& filename,
                                           std::string_view frameID) {
	try {
		const tagReader TagR{ filename, frameID };
		
		return expected::makeValue<std::string>(TagR.getFramePayload());
	}
	catch (id3::audio_tag_error& e) {
		//std::cerr << e.what();
		return expected::makeError<std::string>("Not found");
	}
}

const std::optional<bool> setFramePayload(const std::string& filename, std::string_view frameID,
    std::string_view framePayload) {

	try {
		const tagReader TagR{filename, frameID};

		return TagR.writeFramePayload(framePayload, 0, framePayload.size());
	}
	catch (const id3::audio_tag_error& e) {
		return false;
	}
}

const expected::Result<std::string> GetTitle(const std::string& filename) {
    std::string_view frameID("TITLE");

    return getFramePayload(filename, frameID);
}

const expected::Result<std::string> GetLeadArtist(const std::string& filename) {
    std::string_view frameID("ARTIST");

    return getFramePayload(filename, frameID);
}

const expected::Result<std::string> GetAlbum(const std::string& filename) {
    std::string_view frameID("ALBUM");

    return getFramePayload(filename, frameID);
}

const expected::Result<std::string> GetYear(const std::string& filename) {
    return getFramePayload(filename, std::string_view("YEAR"));
}

const expected::Result<std::string> GetComment(const std::string& filename) {
    return getFramePayload(filename, std::string_view("COMMENT"));
}

const expected::Result<std::string> GetGenre(const std::string& filename) {
    return getFramePayload(filename, std::string_view("GENRE"));
}

const expected::Result<std::string> GetComposer(const std::string& filename) {
    return getFramePayload(filename, std::string_view("COMPOSER"));
}

const std::optional<bool> SetTitle(const std::string& filename,
                                      std::string_view content) {
    return ape::setFramePayload(filename, std::string_view("TITLE"), content);
}

const std::optional<bool> SetAlbum(const std::string& filename,
                                      std::string_view content) {
    return ape::setFramePayload(filename, std::string_view("ALBUM"), content);
}

const std::optional<bool> SetLeadArtist(const std::string& filename,
                                           std::string_view content) {
    return ape::setFramePayload(filename, std::string_view("ARTIST"), content);
}

const std::optional<bool> SetYear(const std::string& filename,
                                     std::string_view content) {
    return ape::setFramePayload(filename, std::string_view("YEAR"), content);
}

const std::optional<bool> SetComment(const std::string& filename,
                                        std::string_view content) {
    return ape::setFramePayload(filename, std::string_view("COMMENT"), content);
}

const std::optional<bool> SetGenre(const std::string& filename,
                                      std::string_view content) {
    return ape::setFramePayload(filename, std::string_view("GENRE"), content);
}

const std::optional<bool> SetComposer(const std::string& filename,
                                         std::string_view content) {
    return ape::setFramePayload(filename, std::string_view("COMPOSER"), content);
}

};  // end namespace ape

#endif  // APE_BASE

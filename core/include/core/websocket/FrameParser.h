#pragma once

#include "core/websocket/Websocket.h"

namespace core::websocket
{
	class FrameParser
	{
		enum STATE
		{
			STATE_PRE,
			STATE_HEADER,
			STATE_PAYLOAD,
			STATE_END,
		};

		STATE m_state = STATE_PRE;
		size_t m_neededBytes = 2;
		size_t m_payloadLengthSize = 0;
		size_t m_payloadOffset = 0;
		Span<const std::byte> m_mask;
		FrameHeader m_header;
		Buffer m_payload;
		bool m_isFragmented = false;
		Allocator* m_allocator = nullptr;

		void pushPayload(Span<const std::byte> payload);

		Result<bool> step(const std::byte* ptr, size_t size);

	public:
		explicit FrameParser(Allocator* allocator)
			: m_payload(allocator),
			  m_allocator(allocator)
		{}

		size_t neededBytes() const { return m_neededBytes; }

		CORE_EXPORT Result<bool> consume(const std::byte* ptr, size_t size);

		CORE_EXPORT Msg msg() const;
	};
}
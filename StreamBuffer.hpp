/**
 * @file SteramBuffer.h
 * @brief
 * @author Mounmory (237628106@qq.com) https://github.com/Mounmory
 * @date
 *
 *
 */

#ifndef MMR_UTIL_STREAM_BUGGER_H
#define MMR_UTIL_STREAM_BUGGER_H

#include <string>
#include <memory>
#include <cstring>
#include <type_traits>
#include <functional>
#include <cassert>

 /*
	 可以处理数据流的buffer
 */

enum class emEndian //计算机大小端
{
	LITTLE = 1,
	BIG = 0
};


namespace mmrUtil
{

	class StreamBuffer
	{
		using BYTE = uint8_t;
	public:
		StreamBuffer(emEndian streamEndian = emEndian::BIG)
			: m_endianStream(streamEndian)
		{
			initMachineEndian();
		}

		/*
			这个构造函数用于直接使用外部地址数据解析，仅限使用operator >> 将数据赋值给其他数据，指针生命周期由外部控制，确保在整个生命周期指针的有效性。
			即使调用其它接口也是内存安全的，若涉及导致重新分配内存，将拷贝地址上的数据然后丢弃这个地址
		*/
		StreamBuffer(void* buffer, uint32_t length, emEndian streamEndian = emEndian::LITTLE)
			: m_endianStream(streamEndian)
			, m_ptrStart((BYTE*)buffer)//只赋值指针和写位置
			, m_ulWritePos(length)
		{
			initMachineEndian();
		}

		StreamBuffer(const StreamBuffer&) = delete;

		StreamBuffer(StreamBuffer&& rhs)
			: m_endianStream(rhs.m_endianStream)
			, m_endianLocal(rhs.m_endianLocal)
			, m_ulReadPos(std::exchange(rhs.m_ulReadPos, 0))
			, m_ulWritePos(std::exchange(rhs.m_ulWritePos, 0))
			, m_ptrStart(std::exchange(rhs.m_ptrStart, nullptr))
			, m_ulCapacity(std::exchange(rhs.m_ulCapacity, 0))
			, m_ptrBuf(std::exchange(rhs.m_ptrBuf, nullptr)) { }

		StreamBuffer& operator = (const StreamBuffer&) = delete;

		StreamBuffer& operator = (StreamBuffer&& rhs)
		{
			if (this != &rhs)
			{
				m_ulReadPos = std::exchange(rhs.m_ulReadPos, 0);
				m_ulWritePos = std::exchange(rhs.m_ulWritePos, 0);
				m_ptrStart = std::exchange(rhs.m_ptrStart, nullptr);
				m_ulCapacity = std::exchange(rhs.m_ulCapacity, 0);
				m_ptrBuf = std::exchange(rhs.m_ptrBuf, nullptr);
			}
			return *this;
		}

		template<typename _Ty>
		typename std::enable_if<std::is_arithmetic<_Ty>::value && std::is_same<std::decay_t<_Ty>, _Ty>::value, StreamBuffer&>::type
			operator << (_Ty data)
		{
			static constexpr uint32_t dataSize = sizeof(_Ty);
			BYTE* ptrData = reinterpret_cast<BYTE*>(&data);
			doFlip(ptrData, dataSize);
			writeBuf(ptrData, dataSize);
			return *this;
		}

		template<typename _Ty>
		typename std::enable_if<std::is_enum<_Ty>::value && std::is_same<std::decay_t<_Ty>, _Ty>::value, StreamBuffer&>::type
			operator << (_Ty data)
		{
			using Type = std::underlying_type_t<_Ty>;
			(*this) << static_cast<Type>(data);
			return *this;
		}

		template<typename _Ty>
		typename std::enable_if<std::is_arithmetic<_Ty>::value && std::is_same<std::decay_t<_Ty>, _Ty>::value, StreamBuffer&>::type
			operator >> (_Ty& data)
		{
			static constexpr uint32_t dataSize = sizeof(_Ty);
			BYTE* ptrData = reinterpret_cast<BYTE*>(&data);
			readBuf(ptrData, dataSize);
			doFlip(ptrData, dataSize);
			return *this;
		}

		template<typename _Ty>
		typename std::enable_if<std::is_enum<_Ty>::value && std::is_same<std::decay_t<_Ty>, _Ty>::value, StreamBuffer&>::type
			operator >> (_Ty& data)
		{
			using Type = std::underlying_type_t<_Ty>;
			(*this) >> (*reinterpret_cast<Type*>(&data));
			return *this;
		}

		StreamBuffer& operator << (const std::string& strData)
		{
			uint32_t ulSize = strData.size();
			(*this) << ulSize;
			writeBuf(strData.data(), ulSize);
			return *this;
		}

		StreamBuffer& operator >> (std::string& strData)
		{
			uint32_t ulSize;
			(*this) >> ulSize;
			assert(m_ulWritePos >= m_ulReadPos + ulSize);
			strData.resize(ulSize);
			readBuf(const_cast<char*>(strData.data()), ulSize);
			return *this;
		}

		void setCapacity(uint32_t length)
		{
			if (length > m_ulCapacity)
			{
				m_ulCapacity = length;
				auto ptrNe = std::unique_ptr<BYTE, std::function<void(BYTE*)>>(new BYTE[m_ulCapacity], [](BYTE* ptr) {delete[] ptr; });
				memcpy(ptrNe.get(), m_ptrStart, m_ulWritePos);
				m_ptrBuf = std::move(ptrNe);
				m_ptrStart = m_ptrBuf.get();
			}
		}

		//获取当前写位置，并在外部进行内存写入
		BYTE* getWritePosAndWrite(uint32_t length)
		{
			checkBufCapacity(m_ulWritePos + length);
			m_ulWritePos += length;
			return (m_ptrStart + m_ulWritePos - length);
		}

		BYTE* getReadPosAndRead(uint32_t length)
		{
			m_ulReadPos += length;
			assert(m_ulWritePos >= m_ulReadPos);
			return (m_ptrStart + m_ulReadPos - length);
		}

		void doFlip(BYTE* buffer, uint32_t dataSize)
		{
			if (m_endianStream != m_endianLocal && dataSize >= 2)
			{
				BYTE* start = buffer;
				BYTE* end = buffer + (dataSize - 1);
				while (start < end)
				{
					std::swap(*start, *end);
					++start;
					--end;
				}
			}
		}

		void clear()
		{
			m_ulReadPos = 0;
			m_ulWritePos = 0;
		}

		uint32_t dataSize() { return (m_ulWritePos - m_ulReadPos); }

		uint32_t getCapacity()const { return m_ulCapacity; }

		BYTE* getStartPtr()const { return m_ptrStart; }

		uint32_t getReadPos()const { return m_ulReadPos; }

		uint32_t getWritePos()const { return m_ulWritePos; }
	private:
		void writeBuf(const void* buffer, uint32_t length)
		{
			checkBufCapacity(m_ulWritePos + length);
			memcpy(m_ptrStart + m_ulWritePos, buffer, length);
			m_ulWritePos += length;
		}

		void readBuf(void* buffer, uint32_t length)
		{
			memcpy(buffer, m_ptrStart + m_ulReadPos, length);
			m_ulReadPos += length;
			assert(m_ulWritePos >= m_ulReadPos);
		}

		void checkBufCapacity(uint32_t length)
		{
			if (length > m_ulCapacity)
			{
				setCapacity(2 * length);
			}
		}

		void initMachineEndian()//初始化本地大小端Endian
		{
			long one(1);
			char e = (reinterpret_cast<char*>(&one))[0];
			(e == (char)1) ? m_endianLocal = emEndian::LITTLE : m_endianLocal = emEndian::BIG;
		}
	private:
		const emEndian m_endianStream;//数据流的的大小端
		emEndian m_endianLocal;//本地计算机大小端

		uint32_t m_ulReadPos = 0;//buf读位置
		uint32_t m_ulWritePos = 0;//buf写位置

		BYTE* m_ptrStart = nullptr;//
		uint32_t m_ulCapacity = 0;//bug空间容量
		std::unique_ptr<BYTE, std::function<void(BYTE*)>> m_ptrBuf = nullptr;//buf指针
	};

}
#endif

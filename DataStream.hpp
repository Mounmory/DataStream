/**
 * @file DataStream.hpp
 * @brief 将数据序列化和反序列化的类
 * @author Mounmory (237628106@qq.com) https://github.com/Mounmory
 * @date
 *
 * 源码参考：https://github.com/open-dis/open-dis-cpp/blob/master/src/dis6/utils/DataStream.h
 * 修改内容：
 * - 将类改为模板类，buffer类型作为模板参数
 * - 增加了直接对一些容器类的支持
 * - 增加了直接enum class类型支持
 * - 增加了对buffer移动输出、通过buf构造DataStream数据
 *
 * 待优化：还有一些效率方面可优化
 */

#ifndef MMR_UTIL_DATASTREAM_HPP
#define MMR_UTIL_DATASTREAM_HPP
#include <string>                 // for typedef, member
#include <vector>                 // for typedef, member
#include <cstdlib>                // for size_t and NULL definition
#include <cstring>                // for memcpy
#include <type_traits>

#if _MSC_VER
#pragma warning( push )
#pragma warning( disable : 4251 )
#endif

namespace mmrUtil
{

	enum class emEndian //计算机大小端
	{
		LITTLE = 1,
		BIG = 0,
	};

#define  OPERATOR_WRITE_READ_BASIC_DATA(type)\
DataStream& operator <<(type date)\
{\
	WriteAlgorithm(date);\
	return *this;\
}\
DataStream& operator >> (type& date) \
{\
	ReadAlgorithm(date);\
	return *this;\
}

	template<typename _BufferType = std::vector<char>>
	class DataStream
	{
		static_assert(
			std::is_same<_BufferType, std::vector<char>>::value ||
			//std::is_same<_BufferType, std::vector<unsigned char>>::value ||
			std::is_same<_BufferType, std::string>::value,
			"buf data type error!");
	public:
		DataStream(emEndian stream)
			: _buffer()
			, _read_pos(0)
			, _stream_endian(stream)
			, _machine_endian(emEndian::LITTLE)
		{
			initMachineEndian();
		}

		DataStream(const char* buffer, size_t length, emEndian stream)
			: _buffer()
			, _read_pos(0)
			, _stream_endian(stream)
			, _machine_endian(emEndian::LITTLE)
		{
			initMachineEndian();

			SetStream(buffer, length, stream);
		}

		DataStream(_BufferType bufData, emEndian stream)
			: _buffer(std::move(bufData))
			, _read_pos(0)
			, _stream_endian(stream)
			, _machine_endian(emEndian::LITTLE)
		{
			initMachineEndian();
		}

		~DataStream() {}

		const auto& operator [](uint32_t offset) const { return _buffer[_read_pos + offset]; }

		/// take ownership of the data buffer.
		void SetStream(const char* buffer, size_t length, emEndian order)
		{
			_stream_endian = order;
			_read_pos = 0;
			_buffer.resize(length);
			memcpy(&_buffer[0], buffer, length);
		}

		// write operations
		OPERATOR_WRITE_READ_BASIC_DATA(char)

		OPERATOR_WRITE_READ_BASIC_DATA(unsigned char)

		OPERATOR_WRITE_READ_BASIC_DATA(float)

		OPERATOR_WRITE_READ_BASIC_DATA(double)

		OPERATOR_WRITE_READ_BASIC_DATA(int16_t)

		OPERATOR_WRITE_READ_BASIC_DATA(uint16_t)

		OPERATOR_WRITE_READ_BASIC_DATA(int32_t)

		OPERATOR_WRITE_READ_BASIC_DATA(uint32_t)

		OPERATOR_WRITE_READ_BASIC_DATA(int64_t)

		OPERATOR_WRITE_READ_BASIC_DATA(uint64_t)

			//处理枚举类型
		template<typename _Ty, typename = std::enable_if_t<std::is_enum<_Ty>::value>>
		DataStream& operator << (_Ty emValue)
		{
			(*this) << static_cast<std::underlying_type_t<_Ty>>(emValue);
			return *this;
		}

		template<typename _Ty, typename = std::enable_if_t<std::is_enum<_Ty>::value>>
		DataStream& operator >> (_Ty& emValue)
		{
			using Type = std::underlying_type_t<_Ty>;
			(*this) >> (*reinterpret_cast<Type*>(&emValue));
			return *this;
		}

		DataStream& operator << (const std::string& str)
		{
			uint32_t strLen = str.size();
			(*this) << strLen;

			_buffer.insert(_buffer.end(), str.begin(), str.begin() + strLen);
			return *this;
		}

		DataStream& operator >> (std::string& str)
		{
			uint32_t strLen(0);
			(*this) >> strLen;

			str.resize(strLen);
			memcpy((void*)str.data(), _buffer.data() + _read_pos, strLen);
			_read_pos += strLen;
			return *this;
		}

		emEndian GetStreamEndian() const { return _stream_endian; }

		emEndian GetMachineEndian() const { return _machine_endian; }

		size_t GetReadPos() const { return _read_pos; }

		const _BufferType& GetBuffer() const { return _buffer; }

		_BufferType moveBuffer()
		{
			_read_pos = 0;
			return std::move(_buffer);
		}

		size_t size() const { return _buffer.size(); }

		void clear()
		{
			_read_pos = 0;
			_buffer.clear();
		}

		bool empty() const { return _buffer.empty(); }

		const auto data() const { return _buffer.data(); }

	private:
		void initMachineEndian()
		{
			long one(1);
			char e = (reinterpret_cast<char*>(&one))[0];
			(e == (char)1) ? _machine_endian = emEndian::LITTLE : _machine_endian = emEndian::BIG;
		}

		template<typename T, typename IterT>
		void IncrementPointer(IterT& iter)
		{
			iter += sizeof(T);
		}

		template<typename T, typename IterT>
		void DecrementPointer(IterT& iter)
		{
			iter -= sizeof(T);
		}

		template<typename T>
		void WriteAlgorithm(T t)
		{
			char* ch = reinterpret_cast<char*>(&t);
			DoFlip(ch, sizeof(T));
			DoWrite(ch, sizeof(T));
		}

		template<typename T>
		void ReadAlgorithm(T& t)
		{
			char ch[sizeof(T)];
			DoRead(ch, sizeof(T));
			DoFlip(ch, sizeof(T));
			memcpy(&t, ch, sizeof(t));
			IncrementPointer<T>(_read_pos);
		}

		/// will flip the buffer if the buffer endian is different than the machine's.
		void DoFlip(char* buf, size_t bufsize)
		{
			if (_machine_endian == _stream_endian || bufsize < 2)
			{
				return;
			}
			// flip it, this fills back to front
			char* start = &buf[0];
			char* end = &buf[bufsize - 1];
			while (start < end)
			{
				char temp = *start;
				*start = *end;
				*end = temp;
				++start;
				--end;
			}
		}

		void DoWrite(const char* buf, size_t bufsize) { _buffer.insert(_buffer.end(), buf, buf + bufsize); }

		void DoRead(char* ch, size_t bufsize) { memcpy(ch, &_buffer.at(_read_pos), bufsize); }

		_BufferType _buffer;

		size_t _read_pos;// the location of the read/write.

		emEndian _stream_endian;// the requirement for the managed buffer

		emEndian _machine_endian;// the native endian type
	};

}

#if _MSC_VER
#pragma warning( pop ) 
#endif

#endif  // MMR_UTIL_DATASTREAM_HPP

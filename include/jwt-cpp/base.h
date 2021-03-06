#pragma once
#include "errors.h"
#include <string>
#include <array>

namespace jwt
{
	namespace alphabet
	{
		struct base64
		{
			static const std::array<char, 64>& data()
			{
				static std::array<char, 64> data = {{'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M',
				                                     'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z',
				                                     'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm',
				                                     'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z',
				                                     '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '+', '/'}};
				return data;
			};
			static const std::string& fill()
			{
				static std::string fill = "=";
				return fill;
			}
		};
		struct base64url
		{
			static const std::array<char, 64>& data()
			{
				static std::array<char, 64> data = {{'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M',
				                                     'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z',
				                                     'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm',
				                                     'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z',
				                                     '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '-', '_'}};
				return data;
			};
			static const std::string& fill()
			{
				static std::string fill = "%3d";
				return fill;
			}
		};
	} // namespace alphabet

	class base
	{
	public:
		template <typename T>
		static result_t<std::string> encode(const std::string& bin)
		{
			return encode(bin, T::data(), T::fill());
		}
		template <typename T>
		static result_t<std::string> decode(const std::string& base)
		{
			return decode(base, T::data(), T::fill());
		}

	private:
		static result_t<std::string>
		encode(const std::string& bin, const std::array<char, 64>& alphabet, const std::string& fill)
		{
			result_t<std::string> res;
			size_t size = bin.size();

			// clear incomplete bytes
			size_t fast_size = size - size % 3;
			for (size_t i = 0; i < fast_size;)
			{
				uint32_t octet_a = (unsigned char)bin[i++];
				uint32_t octet_b = (unsigned char)bin[i++];
				uint32_t octet_c = (unsigned char)bin[i++];

				uint32_t triple = (octet_a << 0x10) + (octet_b << 0x08) + octet_c;

				res.first += alphabet[(triple >> 3 * 6) & 0x3F];
				res.first += alphabet[(triple >> 2 * 6) & 0x3F];
				res.first += alphabet[(triple >> 1 * 6) & 0x3F];
				res.first += alphabet[(triple >> 0 * 6) & 0x3F];
			}

			if (fast_size == size)
				return res;

			size_t mod = size % 3;

			uint32_t octet_a = fast_size < size ? (unsigned char)bin[fast_size++] : 0;
			uint32_t octet_b = fast_size < size ? (unsigned char)bin[fast_size++] : 0;
			uint32_t octet_c = fast_size < size ? (unsigned char)bin[fast_size++] : 0;

			uint32_t triple = (octet_a << 0x10) + (octet_b << 0x08) + octet_c;

			switch (mod)
			{
				case 1:
					res.first += alphabet[(triple >> 3 * 6) & 0x3F];
					res.first += alphabet[(triple >> 2 * 6) & 0x3F];
					res.first += fill;
					res.first += fill;
					break;
				case 2:
					res.first += alphabet[(triple >> 3 * 6) & 0x3F];
					res.first += alphabet[(triple >> 2 * 6) & 0x3F];
					res.first += alphabet[(triple >> 1 * 6) & 0x3F];
					res.first += fill;
					break;
				default:
					break;
			}

			return res;
		}
		static result_t<std::string> decode(const std::string& base, const std::array<char, 64>& alphabet, const std::string& fill)
		{
			result_t<std::string> res;
			size_t size = base.size();

			size_t fill_cnt = 0;
			while (size > fill.size())
			{
				if (base.substr(size - fill.size(), fill.size()) == fill)
				{
					fill_cnt++;
					size -= fill.size();
					if (fill_cnt > 2)
					{
						res.second = JwtErrc::InvalidInputError;
						return res;
					}
				}
				else
					break;
			}

			if ((size + fill_cnt) % 4 != 0)
			{
				res.second = JwtErrc::InvalidInputError;
				return res;
			}

			size_t out_size = size / 4 * 3;
			res.first.reserve(out_size);

			auto get_sextet = [&](size_t offset) {
				result_t<size_t> res;
				for (size_t i = 0; i < alphabet.size(); i++)
				{
					if (alphabet[i] == base[offset])
					{
						res.first = i;
						return res;
					}
				}
				res.second = JwtErrc::InvalidInputError;
				return res;
			};


			size_t fast_size = size - size % 4;
			for (size_t i = 0; i < fast_size;)
			{
				result_t<uint32_t> sextet_a = get_sextet(i++);
				result_t<uint32_t> sextet_b = get_sextet(i++);
				result_t<uint32_t> sextet_c = get_sextet(i++);
				result_t<uint32_t> sextet_d = get_sextet(i++);

				if (sextet_a.second != JwtErrc::NoError || sextet_b.second != JwtErrc::NoError ||
				    sextet_c.second != JwtErrc::NoError || sextet_d.second != JwtErrc::NoError)
				{
					res.second = JwtErrc::InvalidInputError;
					return res;
				}

				uint32_t triple = (sextet_a.first << 3 * 6) + (sextet_b.first << 2 * 6) + (sextet_c.first << 1 * 6) +
				                  (sextet_d.first << 0 * 6);

				res.first += (triple >> 2 * 8) & 0xFF;
				res.first += (triple >> 1 * 8) & 0xFF;
				res.first += (triple >> 0 * 8) & 0xFF;
			}

			if (fill_cnt == 0)
				return res;

			result_t<size_t> st1 = get_sextet(fast_size);
			if (st1.second != JwtErrc::NoError)
			{
				res.second = st1.second;
				return res;
			}

			result_t<size_t> st2 = get_sextet(fast_size + 1);
			if (st2.second != JwtErrc::NoError)
			{
				res.second = st2.second;
				return res;
			}

			uint32_t triple = (st1.first << 3 * 6) + (st2.first << 2 * 6);

			switch (fill_cnt)
			{
				case 1:
				{
					result_t<size_t> st3 = get_sextet(fast_size + 2);
					if (st3.second != JwtErrc::NoError)
					{
						res.second = st3.second;
						return res;
					}
					triple |= (st3.first << 1 * 6);
					res.first += (triple >> 2 * 8) & 0xFF;
					res.first += (triple >> 1 * 8) & 0xFF;
				}
				break;
				case 2:
				{
					res.first += (triple >> 2 * 8) & 0xFF;
				}
				break;
				default:
					break;
			}

			return res;
		}
	};
} // namespace jwt

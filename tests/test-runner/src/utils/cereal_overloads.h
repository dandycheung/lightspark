/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2024  mr b0nk 500 (b0nk@b0nk.xyz)
    Copyright (C) 2025  Ludger Krämer <dbluelle@onlinehome.de>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
**************************************************************************/

#ifndef UTILS_CEREAL_OVERLOADS_H
#define UTILS_CEREAL_OVERLOADS_H 1

#include <cstdint>
#include <utility>

#include <cereal/cereal.hpp>
#include <cereal/details/traits.hpp>
#include <cereal_inline.hpp>

#include "comp_time_switch.h"
#include "enum_name.h"
#include "variant_name.h"
#include "utils.h"

using namespace lightspark;

template<class Archive>
struct SaveVisitor
{
	Archive& archive;
	SaveVisitor(Archive& _archive) : archive(_archive) {}

	template<typename T, std::enable_if_t<!std::is_empty<T>::value, bool> = false>
	void operator()(T& data) { cereal::save_inline(archive, data); }
	template<typename T, std::enable_if_t<std::is_empty<T>::value, bool> = false>
	void operator()(T& data) {}
};

template<class Archive, typename Variant>
struct LoadVariantImpl
{
	Archive& archive;
	Variant& variant;
	template<size_t I, size_t N>
	size_t operator()(Size<I>, Size<N>)
	{
		variant.template emplace<N>();
		cereal::load_inline(archive, std::get<N>(variant));
		return N;
	}
};

template<typename Variant, class Archive>
void loadVariant(Archive& archive, Variant& variant, size_t target)
{
	constexpr size_t variantSize = std::variant_size_v<Variant>;
	(void)constexprSwitch(target, std::make_index_sequence<variantSize>{}, LoadVariantImpl<Archive, Variant> { archive, variant }, [] { return -1; });
}

namespace cereal
{
	using namespace traits;

	template<class Archive, class T, class U>
	void save(Archive& archive, const std::pair<T, U>& pair)
	{
		archive(cereal::make_size_tag(2));
		archive(pair.first, pair.second);
	}

	template<class Archive, class T, class U>
	void load(Archive& archive, std::pair<T, U>& pair)
	{
		size_t size;
		archive(cereal::make_size_tag(size));
		archive(pair.first, pair.second);
	}

	template<class Archive>
	std::string CEREAL_SAVE_MINIMAL_FUNCTION_NAME(const Archive& archive, const std::wstring& wstr)
	{
		return fromWString<wchar_t>(wstr);
	}
	template<class Archive>
	void CEREAL_LOAD_MINIMAL_FUNCTION_NAME(const Archive& archive, std::wstring& wstr, const std::string& str)
	{
		wstr = toWString<wchar_t>(str);
	}

	template<class Archive, class T, std::enable_if_t<std::is_enum<T>::value, bool> = false>
	std::string save_minimal(Archive& archive, const T& type)
	{
		return std::string(enumName(type));
	}
	
	template<class Archive, class T, std::enable_if_t<std::is_enum<T>::value, bool> = false>
	void load_minimal(const Archive& archive, T& type, const std::string& str)
	{
		type = enumCast<T>(str);
	}

	template<class Archive>
	std::string save_minimal(const Archive&, const std::string_view& view)
	{
		return std::string(view.data(), view.size());
	}

	template<class Archive>
	void load_minimal(const Archive&, std::string_view& view, const std::string& str)
	{
		view = str;
	}

	template<class Archive, typename T>
	std::basic_string<T> save_minimal(Archive& archive, const std::basic_string_view<T>& view)
	{
		return std::basic_string<T>(view.data(), view.size());
	}

	template<class Archive, typename T>
	void load_minimal(Archive& archive, std::basic_string_view<T>& view, const std::basic_string<T>& str)
	{
		view = str;
	}

	template<class Archive, typename... Types>
	void save(Archive& archive, const std::variant<Types...>& variant)
	{
		TypeName type = toVariantName(variant);
		archive(CEREAL_NVP(type));
		SaveVisitor<Archive> visitor(archive);
		std::visit(visitor, variant);
	}

	template<class Archive, typename... Types>
	void load(Archive& archive, std::variant<Types...>& variant)
	{
		TypeName type;
		archive(CEREAL_NVP(type));
#ifdef NDEBUG
		loadVariant(archive, variant, fromVariantName(variant, type));
#else
		// HACK for some reason the string_view "type" is made invalid during fromVariantName() when in debug mode.
		// There seems to be a problem with allocating memory on the heap (even malloc doesn't work),
		// so we allocate memory on stack
		// this does only work on gcc, not on llvm
		auto i=type.size();
		char* c=new char[i];
		memcpy(c,type.data(),i);
		std::string s(c,i);
		loadVariant(archive, variant, fromVariantName(variant, s));
		delete[] c;
#endif
	}

	template<class Archive, typename T>
	void save(Archive& archive, const std::optional<T>& optional)
	{
		if (optional.has_value())
			cereal::save_inline(archive, *optional);
	}
	template<class Archive, typename T>
	void load(Archive& archive, std::optional<T>& optional)
	{
		try
		{
			optional.emplace();
			cereal::load_inline(archive, *optional);
		}
		catch (std::exception& e)
		{
			optional.reset();
		}
	}
};

template<class Archive, class T>
void prologue(Archive&, std::vector<T>&) {}
template<class Archive, class T>
void epilogue(Archive&, std::vector<T>&) {}

#endif /* UTILS_CEREAL_OVERLOADS_H */

#pragma once

#include <optional>
#include "source_builder.hpp"
#include "parsing.hpp"

basic_builder& make_load_record(basic_builder& o, file_def const & fd);
basic_builder& make_id_definition(basic_builder& o, std::string const& type_name, std::string const& underlying_type);
basic_builder& make_value_to_vector_type(basic_builder& o, std::string const& qualified_name);

enum class struct_padding { none, fixed, calculated };

basic_builder& make_member_container(basic_builder& o,
	std::string const& member_name, std::string const& type_name, std::string const& size, struct_padding pad, bool is_expandable,
	std::optional<std::string> const& special_fill = std::optional<std::string>());
std::string expand_size_to_fill_cacheline_calculation(std::string const& member_type, size_t base_size);

basic_builder& make_erasable_object_constructor(basic_builder& o, std::string const& name, size_t size);

enum class hook_type { bitfield, vectorizable, other };
basic_builder& make_hooked_getters(basic_builder& o, std::string const& object_name, std::string const& property_name, 
	std::string const& property_type, hook_type ht, bool is_expandable);
basic_builder& make_hooked_setters(basic_builder& o, std::string const& object_name, std::string const& property_name,
	std::string const& property_type, hook_type ht, bool is_expandable);

basic_builder& make_bitfield_getters(basic_builder& o, std::string const& object_name, std::string const& property_name,
	bool is_expandable);
basic_builder& make_bitfield_setters(basic_builder& o, std::string const& object_name, std::string const& property_name,
	bool is_expandable);
basic_builder& make_object_getters(basic_builder& o, std::string const& object_name,  std::string const& property_name,
	std::string const& type);
basic_builder& make_object_setters(basic_builder& o, std::string const& object_name, std::string const& property_name,
	std::string const& type);
basic_builder& make_vectorizable_type_getters(basic_builder& o, std::string const& object_name, std::string const& property_name,
	std::string const& type, bool is_expandable);
basic_builder& make_vectorizable_type_setters(basic_builder& o, std::string const& object_name, std::string const& property_name,
	std::string const& type, bool is_expandable);
basic_builder& make_special_array_getters(basic_builder& o, std::string const& object_name, std::string const& property_name,
	std::string const& type);
basic_builder& make_special_array_setters(basic_builder& o, std::string const& object_name, std::string const& property_name,
	std::string const& type);


//
// This file provided as part of the DataContainer project
//

#include <string>
#include <vector>
#include <fstream>
#include <filesystem> 
#include <iostream> 
#include <algorithm>

#include "parsing.hpp"
#include "code_fragments.hpp"



[[noreturn]] void error_to_file(std::string const& file_name, std::string const& message) {
	std::fstream fileout;
	fileout.open(file_name, std::ios::out);
	if(fileout.is_open()) {
		fileout << message;
		fileout.close();
	}
	std::abort();
}

relationship_object_def const* better_primary_key(relationship_object_def const* oldr, relationship_object_def const* newr) {
	if(oldr == nullptr) {
		return newr;
	}

	if(oldr->is_expandable && !newr->is_expandable)
		return newr;
	if(!oldr->is_expandable && newr->is_expandable)
		return oldr;

	switch(oldr->store_type) {
		case storage_type::contiguous:
		{
			switch(newr->store_type) {
				case storage_type::contiguous:
					return oldr->size <= newr->size ? oldr : newr;
				case storage_type::compactable:
				case storage_type::erasable:
					return oldr;
			}
			break;
		}
		case storage_type::erasable:
		{
			switch(newr->store_type) {
				case storage_type::contiguous:
					return newr;
				case storage_type::erasable:
					return oldr->size <= newr->size ? oldr : newr;
				case storage_type::compactable:
					return oldr;
			}
			break;
		}
		case storage_type::compactable:
		{
			switch(newr->store_type) {
				case storage_type::contiguous:
				case storage_type::erasable:
					return newr;
				case storage_type::compactable:
					return oldr->size <= newr->size ? oldr : newr;
			}
			break;
		}
	}

	return oldr;
}

int main(int argc, char *argv[]) {
	if(argc > 1) {
		std::fstream input_file;
		input_file.open(argv[1], std::ios::in);

		const std::string output_file_name = [otemp = std::string(argv[1])]() mutable {
			if(otemp.length() >= 4 && otemp[otemp.length() - 4] == '.') {
				otemp[otemp.length() - 3] = 'h';
				otemp[otemp.length() - 2] = 'p';
				otemp[otemp.length() - 1] = 'p';
				return otemp;
			}
			return otemp + ".cpp";
		}();


		if(!input_file.is_open()) {
			error_to_file(output_file_name, std::string("Could not open input file: ") + argv[1]);
		}

		
		error_record err;
		
		std::string file_contents((std::istreambuf_iterator<char>(input_file)), std::istreambuf_iterator<char>());
		
		file_def parsed_file = parse_file(file_contents.c_str(), file_contents.c_str() + file_contents.length(), err);

		if(err.accumulated.length() > 0)
			error_to_file(output_file_name, err.accumulated);

		input_file.close();

		// patchup relationship pointers & other information

		for(auto& r : parsed_file.relationship_objects) {
			if(r.is_relationship) {
				for(auto& l : r.indexed_objects) {
					if(auto linked_object = find_by_name(parsed_file, l.type_name); linked_object) {
						l.related_to = linked_object;
					} else {
						error_to_file(output_file_name, std::string("Could not find object named: ") + l.type_name + " in relationship: " + r.name);
					}
					if(l.index == index_type::at_most_one && !l.is_optional) {
						r.primary_key.points_to = better_primary_key(r.primary_key.points_to, l.related_to);
						if(r.primary_key.points_to == l.related_to)
							r.primary_key.property_name = l.property_name;
					}
				}

				

				if(r.indexed_objects.size() <= 1) {
					error_to_file(output_file_name, std::string("Relationship: ") + r.name + " is between too few objects");
				}

				
				if(r.force_pk.length() > 0) {
					bool pk_forced = false;
					for(auto& link : r.indexed_objects) {
						if(link.property_name == r.force_pk && link.index == index_type::at_most_one) {
							r.primary_key.points_to = link.related_to;
							r.primary_key.property_name = link.property_name;
							pk_forced = true;
						}
					}
					if(!pk_forced) {
						error_to_file(output_file_name, std::string("Was unable to use ") + r.force_pk + std::string(" as a primary key for relationship: ") + r.name);
					}
				}

				for(auto& link : r.indexed_objects) {
					if(link.index != index_type::none)
						link.related_to->relationships_involved_in.push_back(in_relation_information{ r.name, link.property_name,
							r.primary_key.points_to == link.related_to, link.is_optional, link.index, link.ltype, &r });
				}

				if(r.primary_key.points_to) {
					r.size = r.primary_key.points_to->size;
					r.store_type = storage_type::contiguous;
					r.is_expandable = r.primary_key.points_to->is_expandable;
				} else {
					if(r.store_type != storage_type::erasable && r.store_type != storage_type::compactable)
						error_to_file(output_file_name, std::string("Relationship ") + r.name + 
							" has no primary key, and thus must have either a compactable or erasable storage type to provide a delete function.");
				}

			} // end if is a relationship
		} // patchup relationship pointers loop

		// compile list of objects that need serailization stubs
		std::vector<std::string> needs_serialize;
		std::vector<std::string> needs_load_only;

		for(auto& r : parsed_file.relationship_objects) {
			for(auto& p : r.properties) {
				if(p.type == property_type::object) {
					if(std::find(needs_serialize.begin(), needs_serialize.end(), p.data_type) == needs_serialize.end()) {
						needs_serialize.push_back(p.data_type);
						parsed_file.object_types.push_back(p.data_type);
					}
				}
			}
		}
		for(auto &lt : parsed_file.legacy_types) {
			if(std::find(needs_serialize.begin(), needs_serialize.end(), lt) == needs_serialize.end()
				&& std::find(needs_load_only.begin(), needs_load_only.end(), lt) == needs_load_only.end()) {
				needs_load_only.push_back(lt);
				parsed_file.object_types.push_back(lt);
			}
		}

		// identify vectorizable types
		for(auto& ob : parsed_file.relationship_objects) {
			for(auto& prop : ob.properties) {
				if(prop.type == property_type::other && is_vectorizable_type(parsed_file, prop.data_type))
					prop.type = property_type::vectorizable;
				if(prop.type == property_type::array_other && is_vectorizable_type(parsed_file, prop.data_type))
					prop.type = property_type::array_vectorizable;
			}
		}


		// patch up composite key info
		bool needs_hash_include = false;
		std::vector<int32_t> byte_sizes_need_hash;

		for(auto& ob : parsed_file.relationship_objects) {
			for(auto& cc : ob.composite_indexes) {
				needs_hash_include = true;

				int32_t bits_so_far = 0;
				for(auto& k : cc.component_indexes) {

					for(auto& ri : ob.indexed_objects) {
						if(ri.property_name == k.property_name) {
							k.object_type = ri.type_name;
							ri.is_covered_by_composite_key = true;
						}
					}

					if(k.object_type.length() == 0) {
						error_to_file(output_file_name, std::string("Indexed link ") + k.property_name + " in composite key " + cc.name +
							" in relationship " + ob.name + " does not refer to a link in the relationship.");
					}

					k.bit_position = bits_so_far;
					if(k.property_name == ob.primary_key.property_name)
						cc.involves_primary_key = true;

					if(ob.is_expandable) {
						k.number_of_bits = 32;
						bits_so_far += 32;
					} else {
						k.number_of_bits = 0;
						for(auto sz = ob.size; sz != 0; sz = sz >> 1) {
							++k.number_of_bits;
							++bits_so_far;
						}
					}
				}

				cc.total_bytes = (bits_so_far + 7) / 8;
				if(cc.total_bytes > 8 &&
					std::find(byte_sizes_need_hash.begin(), byte_sizes_need_hash.end(), cc.total_bytes) == byte_sizes_need_hash.end()) {
					byte_sizes_need_hash.push_back(cc.total_bytes);
				}
			}
		}

		// compose contents of generated file
		std::string output;

		basic_builder o;
		//includes
		output += "#pragma once\n";
		output += "\n";
		output += "//\n";
		output += "// This file was automatically generated from: " + std::string(argv[1]) + "\n";
		output += "// EDIT AT YOUR OWN RISK; all changes will be lost upon regeneration\n";
		output += "// NOT SUITABLE FOR USE IN CRITICAL SOFTWARE WHERE LIVES OR LIVELIHOODS DEPEND ON THE CORRECT OPERATION\n";
		output += "//\n";
		output += "\n";
		output += "#include <cstdint>\n";
		output += "#include <cstddef>\n";
		output += "#include <utility>\n";
		output += "#include <vector>\n";
		output += "#include <algorithm>\n";
		output += "#include <cassert>\n";
		output += "#include <memory>\n";
		output += "#include <cstring>\n";
		output += "#include \"common_types.hpp\"\n";
		output += "#ifndef DCON_NO_VE\n";
		output += "#include \"ve.hpp\"\n";
		output += "#endif\n";
		if(needs_hash_include) {
			output += "#include \"unordered_dense.h\"\n";
		}
		for(auto& i : parsed_file.includes) {
			output += "#include ";
			output += i;
			output += "\n";
		}

		//open new namespace
		output += "\n";

		output += "#ifdef NDEBUG\n";
		output += "#ifdef _MSC_VER\n";
		output += "#define DCON_RELEASE_INLINE __forceinline\n";
		output += "#else\n";
		output += "#define DCON_RELEASE_INLINE inline __attribute__((always_inline))\n";
		output += "#endif\n";
		output += "#else\n";
		output += "#define DCON_RELEASE_INLINE inline\n";
		output += "#endif\n";
		output += "#pragma warning( push )\n";
		output += "#pragma warning( disable : 4324 )\n";
		
		output += "\n";
		output += "namespace " + parsed_file.namspace + " {\n";

		//load record type
		output += make_load_record(o, parsed_file).to_string(1);
		

		//id types definitions
		for(auto& ob : parsed_file.relationship_objects) {
			const auto underlying_type = ob.is_expandable ? std::string("uint32_t") : size_to_tag_type(ob.size);
			//id class begin
			output += make_id_definition(o, ob.name + "_id", underlying_type).to_string(1);
		}
		for(auto& mi : parsed_file.extra_ids) {
			output += make_id_definition(o, mi.name, mi.base_type).to_string(1);
		}

		// close namespace briefly
		output += "}\n\n";

		//mark each id as going into a tagged vector
		output += "#ifndef DCON_NO_VE\n";
		output += "namespace ve {\n";
		for(auto& ob : parsed_file.relationship_objects) {
			output += make_value_to_vector_type(o, parsed_file.namspace + "::" + ob.name + "_id").to_string(1);
		}
		for(auto& mi : parsed_file.extra_ids) {
			output += make_value_to_vector_type(o, parsed_file.namspace + "::" + mi.name).to_string(1);
		}
		output += "}\n\n";
		output += "#endif\n";


		//reopen namespace
		output += "namespace " + parsed_file.namspace + " {\n";

		output += "\tnamespace detail {\n";

		//declare hashing functions as needed
		for(int32_t i : byte_sizes_need_hash) {
			o + substitute{"intvalue", std::to_string(i) };
			o + "struct internal_hash_@intvalue@" + class_block{
				o + "using is_avalanching = void;";
				o + "auto operator()(dcon::key_data_extended<@intvalue@>const& e) const noexcept -> uint64_t" + block{
					o + "return ankerl::unordered_dense::detail::wyhash::hash(e.values.data(), @intvalue@);";
				};
			};
			output += o.to_string(2);
		}
		output += "\t}\n\n"; // close namespace detail

		//predeclare data_container
		output += "\tclass data_container;\n\n";

		//write internal classes

		//open internal namespace
		output += "\tnamespace internal {\n";

		for(auto& ob : parsed_file.relationship_objects) {
			//object class begin
			output += "\t\tclass alignas(64) " + ob.name + "_class {\n";

			//begin members declaration

			output += "\t\t\tprivate:\n";
			if(ob.store_type == storage_type::erasable) {
				output += make_member_container(o, "_index", ob.name + "_id",
					std::to_string(ob.size), struct_padding::none, ob.is_expandable).to_string(3);
			}

			for(auto& p : ob.properties) {
				if(p.is_derived) {
					// no data for a derived property
				} else if(p.type == property_type::bitfield) {
					output += make_member_container(o, p.name, "dcon::bitfield_type",
						std::string("((uint32_t(") + std::to_string(ob.size) + " + 7)) / 8ui32 + 63ui32) & ~63ui32",
						struct_padding::fixed, ob.is_expandable).to_string(3);
				} else if(p.type == property_type::object) {
					output += make_member_container(o, p.name, p.data_type,
						std::to_string(ob.size),
						struct_padding::none, ob.is_expandable).to_string(3);
				} else if(p.type == property_type::special_vector) {
					//fill in with special vector type and pool object
					output += make_member_container(o, p.name, "dcon::stable_mk_2_tag",
						std::to_string(ob.size),
						struct_padding::none, ob.is_expandable, "std::numeric_limits<dcon::stable_mk_2_tag>::max()").to_string(3);
					
					output += "\t\t\tdcon::stable_variable_vector_storage_mk_2<" + p.data_type + ", 16, " + std::to_string(p.special_pool_size) + " > " + p.name + "_storage;\n";
				} else if(p.type == property_type::array_bitfield) {
					output += make_array_member_container(o,
						p.name, "dcon::bitfield_type", ob.size,
						ob.is_expandable, true).to_string(3);
				} else if(p.type == property_type::array_other || p.type == property_type::array_vectorizable) {
					output += make_array_member_container(o,
						p.name, p.data_type, ob.size,
						ob.is_expandable, false).to_string(3);
				} else {
					output += make_member_container(o, p.name, p.data_type,
						expand_size_to_fill_cacheline_calculation(p.data_type, ob.size),
						struct_padding::fixed, ob.is_expandable).to_string(3);
				}
			} //end non relationship members

			// begin relationship members
			for(auto& i : ob.indexed_objects) {
				if(ob.primary_key == i) {
					//skip recording index value
				} else {
					output += make_member_container(o, i.property_name, i.type_name + "_id",
						expand_size_to_fill_cacheline_calculation(i.type_name + "_id", ob.size),
						struct_padding::fixed, ob.is_expandable).to_string(3);
				}

				if(i.index == index_type::many) {
					if(i.ltype == list_type::list) {
						// list intrusive links
						output += make_member_container(o, std::string("link_") + i.property_name, ob.name + "_id_pair",
							std::to_string(ob.size),
							struct_padding::none, ob.is_expandable).to_string(3);

						output += make_member_container(o, std::string("head_back_") + i.property_name, ob.name + "_id",
							expand_size_to_fill_cacheline_calculation(ob.name + "_id", i.related_to->size),
							struct_padding::fixed, i.related_to->is_expandable).to_string(3);

					} else if(i.ltype == list_type::std_vector) {
						//array of relation ids in object
						output += make_member_container(o, std::string("array_") + i.property_name,
							std::string("std::vector<") + ob.name + "_id>",
							std::to_string(i.related_to->size),
							struct_padding::none, i.related_to->is_expandable).to_string(3);
					} else if(i.ltype == list_type::array) {
						//array of relation ids in object
						if(!i.related_to->is_expandable) {
							output += make_member_container(o, std::string("array_") + i.property_name,
								"dcon::stable_mk_2_tag",
								std::to_string(i.related_to->size),
								struct_padding::none, i.related_to->is_expandable,
								"std::numeric_limits<dcon::stable_mk_2_tag>::max()").to_string(3);

							output += "\t\t\tdcon::stable_variable_vector_storage_mk_2<" + ob.name + "_id, 8, " + std::to_string(ob.size * 2) + " > "
								+ i.property_name + "_storage;\n";
						} else {
							error_to_file(output_file_name, std::string("Unable to estimate an upper bound on storage space for ") +
								ob.name + " " + i.property_name + " arrays because the target is expandable");
						}

					}
				} else if(i.index == index_type::at_most_one) {
					if(ob.primary_key == i) {
						// no link back for primary key
					} else {
						output += make_member_container(o, std::string("link_back_") + i.property_name, ob.name + "_id",
							expand_size_to_fill_cacheline_calculation(ob.name + "_id", i.related_to->size),
							struct_padding::fixed, i.related_to->is_expandable).to_string(3);
					}
				}
			} // end relationship members

			if(ob.store_type == storage_type::erasable) {
				output += "\t\t\t" + ob.name + "_id first_free = " + ob.name + "_id();\n";
			}

			output += "\t\t\tuint32_t size_used = 0;\n";
			output += "\n";

			// make composite key functions and hashmaps
			for(auto& cc : ob.composite_indexes) {
				output += make_composite_key_declarations(o, ob.name, cc).to_string(3);
			}

			output += "\n";
			output += "\t\t\tpublic:\n";

			// constructor
			if(ob.store_type == storage_type::erasable && !ob.is_expandable) {
				output += make_erasable_object_constructor(o, ob.name, ob.size).to_string(3);
			}

			//object class end
			output += "\t\t\tfriend class data_container;\n";
			output += "\t\t};\n\n";
		}

		//close internal namespace
		output += "\t}\n\n";

		// write declarations for fat_ids
		for(auto& obj : parsed_file.relationship_objects) {
			output += "\tclass " + obj.name + "_const_fat_id;\n";
			output += "\tclass " + obj.name + "_fat_id;\n";
		}
		for(auto& obj : parsed_file.relationship_objects) {
			output += make_fat_id(o, obj, parsed_file).to_string(1);
			output += make_const_fat_id(o, obj, parsed_file).to_string(1);
		}

		//class data_container begin
		output += "\tclass alignas(64) data_container {\n";
		output += "\t\tpublic:\n";
		for(auto& ob : parsed_file.relationship_objects) {
			output += "\t\tinternal::" + ob.name + "_class " + ob.name + ";\n";
		}
		output += "\n";

		for(auto& ob : parsed_file.relationship_objects) {
			output += make_object_member_declarations(o, parsed_file, ob,
				true, false, "", false).to_string(2);

			// creation / deletion / move hook routines
			if(ob.hook_create) {
				output += "\t\tvoid on_create_" + ob.name + "(" + ob.name + "_id id);\n";
			}
			if(ob.hook_delete) {
				output += "\t\tvoid on_delete_" + ob.name + "(" + ob.name + "_id id);\n";
			}
			if(ob.hook_move) {
				output += "\t\tvoid on_move_" + ob.name + "(" + ob.name + "_id new_id," + ob.name + "_id old_id);\n";
			}

			output += "\t\tuint32_t " + ob.name + "_size() const noexcept { return " + ob.name + ".size_used; }\n\n";
		} // end getters / setters creation loop over relationships / objects


		output += "\n";


		// creation / deletion routines
		for(auto& cob : parsed_file.relationship_objects) {
			const std::string id_name = cob.name + "_id";

			if(!cob.is_relationship) {
				if(cob.store_type == storage_type::contiguous || cob.store_type == storage_type::compactable) {
					output += make_pop_back(o, cob).to_string(2);
					output += make_object_resize(o, cob).to_string(2);
					output += make_non_erasable_create(o, cob).to_string(2);

					if(cob.store_type == storage_type::compactable) {
						output += make_compactable_delete(o, cob).to_string(2);
					}

				} else if(cob.store_type == storage_type::erasable) {
					output += make_erasable_delete(o, cob).to_string(2);
					output += make_erasable_create(o, cob).to_string(2);
					output += make_object_resize(o, cob).to_string(2);
				}
			} else if(cob.primary_key.points_to) { // primary key relationship
				output += make_object_resize(o, cob).to_string(2);
				output += make_clearing_delete(o, cob).to_string(2);
				output += make_pop_back(o, cob).to_string(2);


				output += "\t\tprivate:\n";
				output += make_internal_move_relationship(o, cob).to_string(2);
				output += "\t\tpublic:\n";

				output += make_relation_try_create(o, cob).to_string(2);
				output += make_relation_force_create(o, cob).to_string(2);

			} else { // non pk relationship
				if(cob.store_type == storage_type::contiguous || cob.store_type == storage_type::compactable) {
					output += make_pop_back(o, cob).to_string(2);
					output += make_object_resize(o, cob).to_string(2);
					if(cob.store_type == storage_type::compactable) {
						output += make_compactable_delete(o, cob).to_string(2);
					}

					output += make_relation_try_create(o, cob).to_string(2);
					output += make_relation_force_create(o, cob).to_string(2);

				} else if(cob.store_type == storage_type::erasable) {
					output += make_erasable_delete(o, cob).to_string(2);
					output += make_object_resize(o, cob).to_string(2);

					output += make_relation_try_create(o, cob).to_string(2);
					output += make_relation_force_create(o, cob).to_string(2);

				}
			} // end case relationship no primary key
		} // end creation / deletion reoutines creation loop

		//iterate over all routines
		for(auto& cob : parsed_file.relationship_objects) {
			output += make_iterate_over_objects(o, cob).to_string(2);
		}

		//write save and load object stubs
		output += "\n";
		for(auto& ostr : needs_serialize) {
			output += "\t\tuint64_t serialize_size(" + ostr + " const& obj) const;\n";
			output += "\t\tvoid serialize(std::byte*& output_buffer, " + ostr + " const& obj) const;\n";
			output += "\t\tvoid deserialize(std::byte const*& input_buffer, " + ostr + " & obj, std::byte const* end) const;\n";
		}
		for(auto& ostr : needs_load_only) {
			output += "\t\tvoid deserialize(std::byte const*& input_buffer, " + ostr + "& obj, std::byte const* end) const;\n";
		}

		// type conversion stubs
		for(auto& con : parsed_file.conversion_list) {
			if(std::find(parsed_file.object_types.begin(), parsed_file.object_types.end(), con.to) != parsed_file.object_types.end() ||
				std::find(parsed_file.object_types.begin(), parsed_file.object_types.end(), con.from) != parsed_file.object_types.end()) {
				output += "\t\t" + con.to + " convert_type(" + con.from + " const& source, " + con.to + "* overload_selector) const;\n";
			}
		}

		output += "\n";
		//reset function
		output += "\t\tvoid reset() {\n";
		for(auto& cob : parsed_file.relationship_objects) {
			if(cob.is_relationship)
				output += "\t\t\t" + cob.name + "_resize(0);\n";
		}
		for(auto& cob : parsed_file.relationship_objects) {
			if(!cob.is_relationship)
				output += "\t\t\t" + cob.name + "_resize(0);\n";
		}
		output += "\t\t}\n";

		for(auto& cob : parsed_file.relationship_objects) {
			for(auto& cc : cob.composite_indexes) {
				output += make_composite_key_getter(o, cob.name, cc).to_string(2);
			}
		}

		output += "\n";
		//make ve interface
		output += "\t\t#ifndef DCON_NO_VE\n";
		for(auto& ob : parsed_file.relationship_objects) {
			output += "\t\tve::vectorizable_buffer<float, " + ob.name + "_id> " + ob.name + "_make_vectorizable_float_buffer() const noexcept {\n";
			output += "\t\t\treturn ve::vectorizable_buffer<float, " + ob.name + "_id>(" + ob.name + ".size_used);\n";
			output += "\t\t}\n";
			output += "\t\tve::vectorizable_buffer<int32_t, " + ob.name + "_id> " + ob.name + "_make_vectorizable_int_buffer() const noexcept {\n";
			output += "\t\t\treturn ve::vectorizable_buffer<int32_t, " + ob.name + "_id>(" + ob.name + ".size_used);\n";
			output += "\t\t}\n";
			if(!ob.is_expandable) {
				output += "\t\ttemplate<typename F>\n";
				output += "\t\tDCON_RELEASE_INLINE void execute_serial_over_" + ob.name + "(F&& functor) {\n";
				output += "\t\t\tve::execute_serial(" + ob.name + ".size_used, functor);\n";
				output += "\t\t}\n";
				output += "#ifndef VE_NO_TBB\n";
				output += "\t\ttemplate<typename F>\n";
				output += "\t\tDCON_RELEASE_INLINE void execute_parallel_over_" + ob.name + "(F&& functor) {\n";
				output += "\t\t\tve::execute_parallel_exact(" + ob.name + ".size_used, functor);\n";
				output += "\t\t}\n";
				output += "#endif\n";
			} else {
				output += "\t\ttemplate<typename F>\n";
				output += "\t\tDCON_RELEASE_INLINE void execute_serial_over_" + ob.name + "(F&& functor) {\n";
				output += "\t\t\tve::execute_serial_unaligned(" + ob.name + ".size_used, functor);\n";
				output += "\t\t}\n";
				output += "#ifndef VE_NO_TBB\n";
				output += "\t\ttemplate<typename F>\n";
				output += "\t\tDCON_RELEASE_INLINE void execute_parallel_over_" + ob.name + "(F&& functor) {\n";
				output += "\t\t\tve::execute_parallel_unaligned(" + ob.name + ".size_used, functor);\n";
				output += "\t\t}\n";
				output += "#endif\n";
			}
		}
		output += "\t\t#endif\n";

		

		output += "\n";
		
		for(auto& rt : parsed_file.load_save_routines) {
			output += make_serialize_plan_generator(o, parsed_file, rt).to_string(2);
		}
		output += make_serialize_all_generator(o, parsed_file).to_string(2);

		output += make_serialize_size(o, parsed_file).to_string(2);
		output += make_serialize(o, parsed_file).to_string(2);
		output += make_deserialize(o, parsed_file, false).to_string(2);
		output += make_deserialize(o, parsed_file, true).to_string(2);

		//write globals
		output += "\n";
		for(auto& gstr : parsed_file.globals) {
			output += "\t\t" + gstr + "\n";
		}

		//class data_container end
		output += "\t};\n\n";

		
		for(auto& obj : parsed_file.relationship_objects) {
			output += make_fat_id_impl(o, obj, parsed_file).to_string(1);
			output += make_const_fat_id_impl(o, obj, parsed_file).to_string(1);
		}

		//close new namespace
		output += "}\n";

		output += "\n";


		output += "#undef DCON_RELEASE_INLINE\n";
		output += "#pragma warning( pop )\n";

		//newline at end of file
		output += "\n";

		std::fstream fileout;
		fileout.open(output_file_name, std::ios::out);
		if(fileout.is_open()) {
			fileout << output;
			fileout.close();
		} else {
			std::abort();
		}
	}
}

#define CATCH_CONFIG_MAIN 

#include "catch.hpp"
#include "common_types.hpp"
#include "relation.hpp"
#include <array>
TEST_CASE("double relation test", "[relationships_tests]") {
	static_assert(std::is_trivially_copyable_v<dcon::thingyA_id>);
	static_assert(std::is_trivially_copy_constructible_v<dcon::thingyA_id>);
	static_assert(std::is_trivially_move_constructible_v<dcon::thingyA_id>);
	static_assert(std::is_trivially_copy_assignable_v<dcon::thingyA_id>);
	static_assert(std::is_trivially_move_assignable_v<dcon::thingyA_id>);
	static_assert(std::is_trivially_destructible_v<dcon::thingyA_id>);
	static_assert(std::is_nothrow_default_constructible_v<dcon::thingyA_id>);

	REQUIRE(dcon::thingyA_id().index() == -1);

	auto ptr = std::make_unique< dcon::data_container >();
	auto o1 = fatten(*ptr, ptr->create_thingyA());
	auto o2 = fatten(*ptr, ptr->create_thingyA());

	auto r1 = fatten(*ptr, ptr->try_create_relate_same(o1, o2));

	REQUIRE(bool(r1));
	REQUIRE(ptr->relate_same_size() == 2);
	REQUIRE(r1.get_left() == o1);
	REQUIRE(r1.get_right() == o2);


	auto o3 = ptr->create_thingyA();
	auto o4 = ptr->create_thingyA();
	auto o5 = ptr->create_thingyA();
	auto o6 = ptr->create_thingyA();
	auto o7 = ptr->create_thingyA();
	auto o8 = ptr->create_thingyA();

	REQUIRE(bool(ptr->try_create_relate_same(o2, o1)));
	REQUIRE(bool(ptr->try_create_relate_same(o3, o3)));
	auto r4 = fatten(*ptr, ptr->try_create_relate_same(o4, o2));
	REQUIRE(bool(r4));
	REQUIRE(bool(ptr->try_create_relate_same(o5, o2)));
	REQUIRE(bool(ptr->try_create_relate_same(o6, o4)));
	auto r2 = fatten(*ptr, ptr->try_create_relate_same(o7, o8));
	REQUIRE(bool(r2));
	auto r3 = fatten(*ptr, ptr->try_create_relate_same(o8, o5));
	REQUIRE(bool(r3));

	REQUIRE(!bool(ptr->try_create_relate_same(o4, o4)));

	REQUIRE(r4.get_right() == o2);

	REQUIRE(r4 == ptr->force_create_relate_same(o4, o4));

	REQUIRE(r4.get_right() == o4);

	REQUIRE(ptr->get_relate_same_by_joint(o4, o4) == r4);
	REQUIRE(ptr->get_relate_same_by_joint(o7, o8) == r2);
	REQUIRE(ptr->get_relate_same_by_joint(o8, o5) == r3);

	ptr->delete_thingyA(o4);
	REQUIRE(ptr->get_relate_same_by_joint(o4, o5) == r4);
	REQUIRE(ptr->get_relate_same_by_joint(o8, o5) == dcon::relate_same_id());
	REQUIRE(ptr->get_relate_same_by_joint(o4, o4) == dcon::relate_same_id());

	ptr->delete_relate_same(r2);
	REQUIRE(ptr->get_relate_same_by_joint(o7, o8) == dcon::relate_same_id());
}

TEST_CASE("array pool index storage", "[relationships_tests]") {
	auto ptr = std::make_unique< dcon::data_container >();

	auto a1 = fatten(*ptr, ptr->create_thingyA());
	auto a2 = fatten(*ptr, ptr->create_thingyA());
	auto a3 = fatten(*ptr, ptr->create_thingyA());
	auto a4 = fatten(*ptr, ptr->create_thingyA());
	auto a5 = fatten(*ptr, ptr->create_thingyA());
	auto a6 = fatten(*ptr, ptr->create_thingyA());
	auto a7 = fatten(*ptr, ptr->create_thingyA());

	auto b1 = fatten(*ptr, ptr->create_thingyB());
	auto b2 = fatten(*ptr, ptr->create_thingyB());
	auto b3 = fatten(*ptr, ptr->create_thingyB());
	auto b4 = fatten(*ptr, ptr->create_thingyB());

	REQUIRE(bool(ptr->try_create_relate_in_array(a7, b3)));
	REQUIRE(bool(ptr->try_create_relate_in_array(a3, b3)));
	REQUIRE(bool(ptr->try_create_relate_in_array(a2, b2)));
	REQUIRE(bool(ptr->try_create_relate_in_array(a5, b2)));
	REQUIRE(bool(ptr->try_create_relate_in_array(a4, b3)));
	REQUIRE(bool(ptr->try_create_relate_in_array(a1, b1)));

	REQUIRE(!a6.get_relate_in_array().is_valid());

	REQUIRE(a1.get_right_from_relate_in_array() == b1);
	REQUIRE(a2.get_right_from_relate_in_array() == b2);
	REQUIRE(a3.get_right_from_relate_in_array() == b3);
	REQUIRE(a4.get_right_from_relate_in_array() == b3);
	REQUIRE(a5.get_right_from_relate_in_array() == b2);
	REQUIRE(a6.get_right_from_relate_in_array() == dcon::thingyB_id());
	REQUIRE(a7.get_right_from_relate_in_array() == b3);

	{
		std::array<bool, 7> foundvals{false, false, false, false, false, false, false};
		int32_t found_count = 0;

		b3.for_each_left_from_relate_in_array([&](auto fi) {
			foundvals[fi.id.index()] = true;
			found_count++;
		});

		REQUIRE(b3.has_left_from_relate_in_array(a7));

		REQUIRE(foundvals[6]);
		REQUIRE(foundvals[2]);
		REQUIRE(foundvals[3]);
		REQUIRE(found_count == 3);
	}
	{
		std::array<bool, 7> foundvals{ false, false, false, false, false, false, false };
		int32_t found_count = 0;

		b2.for_each_left_from_relate_in_array([&](auto fi) {
			foundvals[fi.id.index()] = true;
			found_count++;
		});

		REQUIRE(b2.has_left_from_relate_in_array(a5));

		REQUIRE(foundvals[4]);
		REQUIRE(foundvals[1]);
		REQUIRE(found_count == 2);
	}
	{
		std::array<bool, 7> foundvals{ false, false, false, false, false, false, false };
		int32_t found_count = 0;

		b1.for_each_left_from_relate_in_array([&](auto fi) {
			foundvals[fi.id.index()] = true;
			found_count++;
		});

		REQUIRE(b1.has_left_from_relate_in_array(a1));

		REQUIRE(foundvals[0]);
		REQUIRE(found_count == 1);
	}

	ptr->delete_thingyA(a2);

	// NOTE ALL IDS HAVE CHANGED AS FOLLOWS:
	// a2 = what was previously a7

	REQUIRE(a1.get_right_from_relate_in_array() == b1);
	REQUIRE(a2.get_right_from_relate_in_array() == b3);
	REQUIRE(a3.get_right_from_relate_in_array() == b3);
	REQUIRE(a4.get_right_from_relate_in_array() == b3);
	REQUIRE(a5.get_right_from_relate_in_array() == b2);
	REQUIRE(a6.get_right_from_relate_in_array() == dcon::thingyB_id());

	{
		std::array<bool, 7> foundvals{ false, false, false, false, false, false, false };
		int32_t found_count = 0;

		b3.for_each_left_from_relate_in_array([&](auto fi) {
			foundvals[fi.id.index()] = true;
			found_count++;
		});

		REQUIRE(b3.has_left_from_relate_in_array(a2));

		REQUIRE(foundvals[1]);
		REQUIRE(foundvals[2]);
		REQUIRE(foundvals[3]);
		REQUIRE(found_count == 3);
	}
	{
		std::array<bool, 7> foundvals{ false, false, false, false, false, false, false };
		int32_t found_count = 0;

		b2.for_each_left_from_relate_in_array([&](auto fi) {
			foundvals[fi.id.index()] = true;
			found_count++;
		});

		REQUIRE(b2.has_left_from_relate_in_array(a5));

		REQUIRE(foundvals[4]);
		REQUIRE(found_count == 1);
	}
	{
		std::array<bool, 7> foundvals{ false, false, false, false, false, false, false };
		int32_t found_count = 0;

		b1.for_each_left_from_relate_in_array([&](auto fi) {
			foundvals[fi.id.index()] = true;
			found_count++;
		});

		REQUIRE(b1.has_left_from_relate_in_array(a1));

		REQUIRE(foundvals[0]);
		REQUIRE(found_count == 1);
	}

	ptr->delete_thingyB(b3);

	REQUIRE(a1.get_right_from_relate_in_array() == b1);
	REQUIRE(a2.get_right_from_relate_in_array() == dcon::thingyB_id());
	REQUIRE(a3.get_right_from_relate_in_array() == dcon::thingyB_id());
	REQUIRE(a4.get_right_from_relate_in_array() == dcon::thingyB_id());
	REQUIRE(a5.get_right_from_relate_in_array() == b2);
	REQUIRE(a6.get_right_from_relate_in_array() == dcon::thingyB_id());

	{
		std::array<bool, 7> foundvals{ false, false, false, false, false, false, false };
		int32_t found_count = 0;

		b2.for_each_left_from_relate_in_array([&](auto fi) {
			foundvals[fi.id.index()] = true;
			found_count++;
		});

		REQUIRE(b2.has_left_from_relate_in_array(a5));

		REQUIRE(foundvals[4]);
		REQUIRE(found_count == 1);
	}
	{
		std::array<bool, 7> foundvals{ false, false, false, false, false, false, false };
		int32_t found_count = 0;

		b1.for_each_left_from_relate_in_array([&](auto fi) {
			foundvals[fi.id.index()] = true;
			found_count++;
		});

		REQUIRE(b1.has_left_from_relate_in_array(a1));

		REQUIRE(foundvals[0]);
		REQUIRE(found_count == 1);
	}
}


TEST_CASE("list index storage", "[relationships_tests]") {
	auto ptr = std::make_unique< dcon::data_container >();

	auto a1 = fatten(*ptr, ptr->create_thingyA());
	auto a2 = fatten(*ptr, ptr->create_thingyA());
	auto a3 = fatten(*ptr, ptr->create_thingyA());
	auto a4 = fatten(*ptr, ptr->create_thingyA());
	auto a5 = fatten(*ptr, ptr->create_thingyA());
	auto a6 = fatten(*ptr, ptr->create_thingyA());
	auto a7 = fatten(*ptr, ptr->create_thingyA());

	auto b1 = fatten(*ptr, ptr->create_thingyB());
	auto b2 = fatten(*ptr, ptr->create_thingyB());
	auto b3 = fatten(*ptr, ptr->create_thingyB());
	auto b4 = fatten(*ptr, ptr->create_thingyB());

	REQUIRE(bool(ptr->try_create_relate_in_list(a7, b3)));
	REQUIRE(bool(ptr->try_create_relate_in_list(a3, b3)));
	REQUIRE(bool(ptr->try_create_relate_in_list(a2, b2)));
	REQUIRE(bool(ptr->try_create_relate_in_list(a5, b2)));
	REQUIRE(bool(ptr->try_create_relate_in_list(a4, b3)));
	REQUIRE(bool(ptr->try_create_relate_in_list(a1, b1)));

	REQUIRE(!a6.get_relate_in_list().is_valid());

	REQUIRE(a1.get_right_from_relate_in_list() == b1);
	REQUIRE(a2.get_right_from_relate_in_list() == b2);
	REQUIRE(a3.get_right_from_relate_in_list() == b3);
	REQUIRE(a4.get_right_from_relate_in_list() == b3);
	REQUIRE(a5.get_right_from_relate_in_list() == b2);
	REQUIRE(a6.get_right_from_relate_in_list() == dcon::thingyB_id());
	REQUIRE(a7.get_right_from_relate_in_list() == b3);

	{
		std::array<bool, 7> foundvals{ false, false, false, false, false, false, false };
		int32_t found_count = 0;

		b3.for_each_left_from_relate_in_list([&](auto fi) {
			foundvals[fi.id.index()] = true;
			found_count++;
		});

		REQUIRE(b3.has_left_from_relate_in_list(a7));

		REQUIRE(foundvals[6]);
		REQUIRE(foundvals[2]);
		REQUIRE(foundvals[3]);
		REQUIRE(found_count == 3);
	}
	{
		std::array<bool, 7> foundvals{ false, false, false, false, false, false, false };
		int32_t found_count = 0;

		b2.for_each_left_from_relate_in_list([&](auto fi) {
			foundvals[fi.id.index()] = true;
			found_count++;
		});

		REQUIRE(b2.has_left_from_relate_in_list(a5));

		REQUIRE(foundvals[4]);
		REQUIRE(foundvals[1]);
		REQUIRE(found_count == 2);
	}
	{
		std::array<bool, 7> foundvals{ false, false, false, false, false, false, false };
		int32_t found_count = 0;

		b1.for_each_left_from_relate_in_list([&](auto fi) {
			foundvals[fi.id.index()] = true;
			found_count++;
		});

		REQUIRE(b1.has_left_from_relate_in_list(a1));

		REQUIRE(foundvals[0]);
		REQUIRE(found_count == 1);
	}

	ptr->delete_thingyA(a2);

	// NOTE ALL IDS HAVE CHANGED AS FOLLOWS:
	// a2 = what was previously a7

	REQUIRE(a1.get_right_from_relate_in_list() == b1);
	REQUIRE(a2.get_right_from_relate_in_list () == b3);
	REQUIRE(a3.get_right_from_relate_in_list() == b3);
	REQUIRE(a4.get_right_from_relate_in_list() == b3);
	REQUIRE(a5.get_right_from_relate_in_list() == b2);
	REQUIRE(a6.get_right_from_relate_in_list() == dcon::thingyB_id());

	{
		std::array<bool, 7> foundvals{ false, false, false, false, false, false, false };
		int32_t found_count = 0;

		b3.for_each_left_from_relate_in_list([&](auto fi) {
			foundvals[fi.id.index()] = true;
			found_count++;
		});

		REQUIRE(b3.has_left_from_relate_in_list(a2));

		REQUIRE(foundvals[1]);
		REQUIRE(foundvals[2]);
		REQUIRE(foundvals[3]);
		REQUIRE(found_count == 3);
	}
	{
		std::array<bool, 7> foundvals{ false, false, false, false, false, false, false };
		int32_t found_count = 0;

		b2.for_each_left_from_relate_in_list([&](auto fi) {
			foundvals[fi.id.index()] = true;
			found_count++;
		});

		REQUIRE(b2.has_left_from_relate_in_list(a5));

		REQUIRE(foundvals[4]);
		REQUIRE(found_count == 1);
	}
	{
		std::array<bool, 7> foundvals{ false, false, false, false, false, false, false };
		int32_t found_count = 0;

		b1.for_each_left_from_relate_in_list([&](auto fi) {
			foundvals[fi.id.index()] = true;
			found_count++;
		});

		REQUIRE(b1.has_left_from_relate_in_list(a1));

		REQUIRE(foundvals[0]);
		REQUIRE(found_count == 1);
	}

	ptr->delete_thingyB(b3);

	REQUIRE(a1.get_right_from_relate_in_list() == b1);
	REQUIRE(a2.get_right_from_relate_in_list() == dcon::thingyB_id());
	REQUIRE(a3.get_right_from_relate_in_list() == dcon::thingyB_id());
	REQUIRE(a4.get_right_from_relate_in_list() == dcon::thingyB_id());
	REQUIRE(a5.get_right_from_relate_in_list() == b2);
	REQUIRE(a6.get_right_from_relate_in_list() == dcon::thingyB_id());

	{
		std::array<bool, 7> foundvals{ false, false, false, false, false, false, false };
		int32_t found_count = 0;

		b2.for_each_left_from_relate_in_list([&](auto fi) {
			foundvals[fi.id.index()] = true;
			found_count++;
		});

		REQUIRE(b2.has_left_from_relate_in_list(a5));

		REQUIRE(foundvals[4]);
		REQUIRE(found_count == 1);
	}
	{
		std::array<bool, 7> foundvals{ false, false, false, false, false, false, false };
		int32_t found_count = 0;

		b1.for_each_left_from_relate_in_list([&](auto fi) {
			foundvals[fi.id.index()] = true;
			found_count++;
		});

		REQUIRE(b1.has_left_from_relate_in_list(a1));

		REQUIRE(foundvals[0]);
		REQUIRE(found_count == 1);
	}
}

TEST_CASE("many many relation test", "[relationships_tests]") {

	auto ptr = std::make_unique< dcon::data_container >();
	auto o1 = fatten(*ptr, ptr->create_thingyA());
	auto o2 = fatten(*ptr, ptr->create_thingyA());
	auto o3 = fatten(*ptr, ptr->create_thingyA());

	auto r1 = fatten(*ptr, ptr->try_create_many_many(o1, o2, o1, o3, o3, o2, o1));
	auto r2 = fatten(*ptr, ptr->try_create_many_many(o2, o2, o3, o3, o3, o2, o2));
	auto r3 = fatten(*ptr, ptr->try_create_many_many(o1, o2, o1, o3, o1, o2, o1));
	auto r4 = fatten(*ptr, ptr->try_create_many_many(o1, o2, o1, o3, o3, o3, o1));

	REQUIRE(bool(r1));
	REQUIRE(bool(r2));
	REQUIRE(bool(r3));
	REQUIRE(bool(r4));

	REQUIRE(ptr->get_many_many_by_joint(o2, o2, o3, o3, o3, o2) == r2);
	REQUIRE(ptr->get_many_many_by_joint(o1, o2, o3, o3, o3, o1) == dcon::many_many_id());

	REQUIRE(ptr->get_many_many_by_joint(o1, o2, o1, o3, o1, o2) == r3);
	REQUIRE(false == bool(ptr->try_create_many_many(o1, o2, o1, o3, o1, o2, o3)));

	auto r5 = fatten(*ptr, ptr->force_create_many_many(o1, o2, o1, o3, o1, o2, o3));

	REQUIRE(bool(r5));

	REQUIRE(r3.is_valid() == false);
	REQUIRE(ptr->get_many_many_by_joint(o1, o2, o1, o3, o1, o2) == r5);

	REQUIRE(r5.try_set_B(o3));
	REQUIRE(ptr->get_many_many_by_joint(o1, o3, o1, o3, o1, o2) == r5);
	REQUIRE(ptr->get_many_many_by_joint(o1, o3, o1, o3, o1, o2) == r5);

	REQUIRE(!r4.try_set_F(o2));

	REQUIRE(r5.get_ignore() == o3);

	r4.set_F(o2);
	REQUIRE(r4.get_F() == o2);
	REQUIRE(!r1.is_valid());
	REQUIRE(ptr->get_many_many_by_joint(o1, o2, o1, o3, o3, o2) == r4);
}

TEST_CASE("non optional behavior (vs optional)", "[relationships_tests]") {

	auto ptr = std::make_unique< dcon::data_container >();

	auto o1 = fatten(*ptr, ptr->create_thingyA());
	auto o2 = fatten(*ptr, ptr->create_thingyA());
	auto o3 = fatten(*ptr, ptr->create_thingyA());
	auto o4 = fatten(*ptr, ptr->create_thingyA());

	auto p1 = fatten(*ptr, ptr->create_thingyB());
	auto p2 = fatten(*ptr, ptr->create_thingyB());
	auto p3 = fatten(*ptr, ptr->create_thingyB());
	auto p4 = fatten(*ptr, ptr->create_thingyB());

	auto r1 = fatten(*ptr, ptr->try_create_relate_as_non_optional(o1, p4));
	auto r2 = fatten(*ptr, ptr->try_create_relate_as_non_optional(o2, p3));
	auto r3 = fatten(*ptr, ptr->try_create_relate_as_non_optional(o3, p1));
	auto r4 = fatten(*ptr, ptr->try_create_relate_as_non_optional(o1, dcon::thingyB_id()));

	REQUIRE(!bool(r4));
	p1.remove_all_relate_as_non_optional();
	REQUIRE(!r3.is_valid());
	r2.set_right(dcon::thingyB_id());
	REQUIRE(!r2.is_valid());
	ptr->delete_thingyB(p4);
	REQUIRE(!r1.is_valid());
}

TEST_CASE("optional behavior (vs non optional)", "[relationships_tests]") {

	auto ptr = std::make_unique< dcon::data_container >();

	auto o1 = fatten(*ptr, ptr->create_thingyA());
	auto o2 = fatten(*ptr, ptr->create_thingyA());
	auto o3 = fatten(*ptr, ptr->create_thingyA());
	auto o4 = fatten(*ptr, ptr->create_thingyA());

	auto p1 = fatten(*ptr, ptr->create_thingyB());
	auto p2 = fatten(*ptr, ptr->create_thingyB());
	auto p3 = fatten(*ptr, ptr->create_thingyB());
	auto p4 = fatten(*ptr, ptr->create_thingyB());

	auto r1 = fatten(*ptr, ptr->try_create_relate_as_optional(o1, p4));
	auto r2 = fatten(*ptr, ptr->try_create_relate_as_optional(o2, p3));
	auto r3 = fatten(*ptr, ptr->try_create_relate_as_optional(o3, p1));
	auto r4 = fatten(*ptr, ptr->try_create_relate_as_optional(o1, dcon::thingyB_id()));

	REQUIRE(bool(r4));
	p1.remove_all_relate_as_non_optional();
	REQUIRE(r3.is_valid());
	r2.set_right(dcon::thingyB_id());
	REQUIRE(r2.is_valid());
	ptr->delete_thingyB(p4);
	REQUIRE(r1.is_valid());
}
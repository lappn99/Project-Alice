#include "province.hpp"
#include "dcon_generated.hpp"
#include "demographics.hpp"
#include "nations.hpp"
#include "system_state.hpp"
#include <vector>
#include "rebels.hpp"

namespace province {

template<typename T>
auto is_overseas(sys::state const& state, T ids) {
	auto owners = state.world.province_get_nation_from_province_ownership(ids);
	auto owner_cap = state.world.nation_get_capital(owners);
	return (state.world.province_get_continent(ids) != state.world.province_get_continent(owner_cap))
		&& (state.world.province_get_connected_region_id(ids) != state.world.province_get_connected_region_id(owner_cap));
}

template<typename F>
void for_each_land_province(sys::state& state, F const& func) {
	int32_t last = state.province_definitions.first_sea_province.index();
	for(int32_t i = 0; i < last; ++i) {
		dcon::province_id pid{ dcon::province_id::value_base_t(i) };
		func(pid);
	}
}

template<typename F>
void ve_for_each_land_province(sys::state& state, F const& func) {
	int32_t last = state.province_definitions.first_sea_province.index();
	ve::execute_serial<dcon::province_id>(uint32_t(last), func);
}

template<typename F>
void for_each_sea_province(sys::state& state, F const& func) {
	int32_t first = state.province_definitions.first_sea_province.index();
	for(int32_t i = first; i < int32_t(state.world.province_size()); ++i) {
		dcon::province_id pid{ dcon::province_id::value_base_t(i) };
		func(pid);
	}
}

template<typename F>
void for_each_province_in_state_instance(sys::state& state, dcon::state_instance_id s, F const& func) {
	auto d = state.world.state_instance_get_definition(s);
	auto o = state.world.state_instance_get_nation_from_state_ownership(s);
	for(auto p : state.world.state_definition_get_abstract_state_membership(d)) {
		if(p.get_province().get_nation_from_province_ownership() == o) {
			func(p.get_province().id);
		}
	}
}
bool nations_are_adjacent(sys::state& state, dcon::nation_id a, dcon::nation_id b) {
	auto it = state.world.get_nation_adjacency_by_nation_adjacency_pair(a, b);
	return bool(it);
}
void update_connected_regions(sys::state& state) {
	if(!state.adjacency_data_out_of_date)
		return;

	state.adjacency_data_out_of_date = false;

	state.world.nation_adjacency_resize(0);

	state.world.for_each_province([&](dcon::province_id id) {
		state.world.province_set_connected_region_id(id, 0);
	});
	// TODO get a better allocator
	static std::vector<dcon::province_id> to_fill_list;
	uint16_t current_fill_id = 0;

	to_fill_list.reserve(state.world.province_size());

	for(int32_t i = state.province_definitions.first_sea_province.index(); i-- > 0; ) {
		dcon::province_id id{ dcon::province_id::value_base_t(i) };
		if(state.world.province_get_connected_region_id(id) == 0) {
			++current_fill_id;

			to_fill_list.push_back(id);

			while(!to_fill_list.empty()) {
				auto current_id = to_fill_list.back();
				to_fill_list.pop_back();

				state.world.province_set_connected_region_id(current_id, current_fill_id);
				for(auto rel : state.world.province_get_province_adjacency(current_id)) {
					if((rel.get_type() & (province::border::coastal_bit | province::border::impassible_bit)) == 0) { // not entering sea, not impassible
						auto owner_a = rel.get_connected_provinces(0).get_nation_from_province_ownership();
						auto owner_b = rel.get_connected_provinces(1).get_nation_from_province_ownership();
						if(owner_a == owner_b) { // both have the same owner
							if(rel.get_connected_provinces(0).get_connected_region_id() == 0)
								to_fill_list.push_back(rel.get_connected_provinces(0));
							if(rel.get_connected_provinces(1).get_connected_region_id() == 0)
								to_fill_list.push_back(rel.get_connected_provinces(1));
						} else {
							state.world.try_create_nation_adjacency(owner_a, owner_b);
						}
					}
				}
			}

			to_fill_list.clear();
		}
	}
}

dcon::province_id pick_capital(sys::state& state, dcon::nation_id n) {
	auto trad_cap = state.world.national_identity_get_capital(state.world.nation_get_identity_from_identity_holder(n));
	if(state.world.province_get_nation_from_province_ownership(trad_cap) == n) {
		return trad_cap;
	}
	dcon::province_id best_choice;
	for(auto prov : state.world.nation_get_province_ownership(n)) {
		if(prov.get_province().get_demographics(demographics::total) > state.world.province_get_demographics(best_choice, demographics::total) && prov.get_province().get_is_owner_core() == state.world.province_get_is_owner_core(best_choice)) {
			best_choice = prov.get_province().id;
		} else if(prov.get_province().get_is_owner_core() && !state.world.province_get_is_owner_core(best_choice)) {
			best_choice = prov.get_province().id;
		}
	}
	return best_choice;
}

void restore_cached_values(sys::state& state) {
	state.world.execute_serial_over_nation([&](auto ids) {
		state.world.nation_set_owned_province_count(ids, ve::int_vector());
	});
	state.world.execute_serial_over_nation([&](auto ids) {
		state.world.nation_set_central_province_count(ids, ve::int_vector());
	});
	state.world.execute_serial_over_nation([&](auto ids) {
		state.world.nation_set_central_blockaded(ids, ve::int_vector());
	});
	state.world.execute_serial_over_nation([&](auto ids) {
		state.world.nation_set_central_rebel_controlled(ids, ve::int_vector());
	});
	state.world.execute_serial_over_nation([&](auto ids) {
		state.world.nation_set_rebel_controlled_count(ids, ve::int_vector());
	});
	state.world.execute_serial_over_nation([&](auto ids) {
		state.world.nation_set_central_ports(ids, ve::int_vector());
	});
	state.world.execute_serial_over_nation([&](auto ids) {
		state.world.nation_set_central_crime_count(ids, ve::int_vector());
	});
	state.world.execute_serial_over_nation([&](auto ids) {
		state.world.nation_set_total_ports(ids, ve::int_vector());
	});
	state.world.execute_serial_over_nation([&](auto ids) {
		state.world.nation_set_occupied_count(ids, ve::int_vector());
	});
	state.world.execute_serial_over_nation([&](auto ids) {
		state.world.nation_set_owned_state_count(ids, ve::int_vector());
	});
	state.world.execute_serial_over_nation([&](auto ids) {
		state.world.nation_set_is_colonial_nation(ids, ve::mask_vector());
	});

	// need to set owner cores first becasue capital selection depends on them

	for(int32_t i = 0; i < state.province_definitions.first_sea_province.index(); ++i) {
		dcon::province_id pid{ dcon::province_id::value_base_t(i) };

		auto owner = state.world.province_get_nation_from_province_ownership(pid);
		if(owner) {
			bool owner_core = false;
			for(auto c : state.world.province_get_core(pid)) {
				if(c.get_identity().get_nation_from_identity_holder() == owner) {
					owner_core = true;
					break;
				}
			}
			state.world.province_set_is_owner_core(pid, owner_core);
		} else {
			state.world.province_set_is_owner_core(pid, false);
		}
	}

	for(auto n : state.world.in_nation) {
		if(n.get_capital().get_nation_from_province_ownership() != n) {
			n.set_capital(pick_capital(state, n));
		}
	}

	for(int32_t i = 0; i < state.province_definitions.first_sea_province.index(); ++i) {
		dcon::province_id pid{ dcon::province_id::value_base_t(i) };

		auto owner = state.world.province_get_nation_from_province_ownership(pid);
		if(owner) {

			state.world.nation_get_owned_province_count(owner) += uint16_t(1);

			bool reb_controlled = bool(state.world.province_get_rebel_faction_from_province_rebel_control(pid));

			if(reb_controlled) {
				state.world.nation_get_rebel_controlled_count(owner) += uint16_t(1);
			}
			if(state.world.province_get_is_coast(pid)) {
				state.world.nation_get_total_ports(owner) += uint16_t(1);
			}
			if(auto c = state.world.province_get_nation_from_province_control(pid); bool(c) && c != owner) {
				state.world.nation_get_occupied_count(owner) += uint16_t(1);
			}
			if(state.world.province_get_is_colonial(pid)) {
				state.world.nation_set_is_colonial_nation(owner, true);
			}
			if(!is_overseas(state, pid)) {
				state.world.nation_get_central_province_count(owner) += uint16_t(1);

				if(military::province_is_blockaded(state, pid)) {
					state.world.nation_get_central_blockaded(owner) += uint16_t(1);
				}
				if(state.world.province_get_is_coast(pid)) {
					state.world.nation_get_central_ports(owner) += uint16_t(1);
				}
				if(reb_controlled) {
					state.world.nation_get_central_rebel_controlled(owner) += uint16_t(1);
				}
				if(state.world.province_get_crime(pid)) {
					state.world.nation_get_central_crime_count(owner) += uint16_t(1);
				}
			}
		}
	}
	state.world.for_each_state_instance([&](dcon::state_instance_id s) {
		auto owner = state.world.state_instance_get_nation_from_state_ownership(s);
		state.world.nation_get_owned_state_count(owner) += uint16_t(1);
		dcon::province_id p;
		for(auto prv : state.world.state_definition_get_abstract_state_membership(state.world.state_instance_get_definition(s))) {
			if(state.world.province_get_nation_from_province_ownership(prv.get_province()) == owner) {
				p = prv.get_province().id;
				break;
			}
		}
		state.world.state_instance_set_capital(s, p);
	});
}


void update_cached_values(sys::state& state) {
	if(!state.national_cached_values_out_of_date)
		return;

	state.national_cached_values_out_of_date = false;

	restore_cached_values(state);
}



void restore_unsaved_values(sys::state& state) {
	for(int32_t i = 0; i < state.province_definitions.first_sea_province.index(); ++i) {
		dcon::province_id pid{ dcon::province_id::value_base_t(i) };

		for(auto adj : state.world.province_get_province_adjacency(pid)) {
			if((state.world.province_adjacency_get_type(adj) & province::border::coastal_bit) != 0) {
				state.world.province_set_is_coast(pid, true);
				break;
			}
		}
	}

	restore_cached_values(state);
}

/*
// We can probably do without this
void update_state_administrative_efficiency(sys::state& state) {

	//- state administrative efficiency: = define:NONCORE_TAX_PENALTY x number-of-non-core-provinces + (bureaucrat-tax-efficiency x total-number-of-primary-or-accepted-culture-bureaucrats / population-of-state)v1 / x (sum-of-the-administrative_multiplier-for-social-issues-marked-as-being-administrative x define:BUREAUCRACY_PERCENTAGE_INCREMENT + define:MAX_BUREAUCRACY_PERCENTAGE)), all clamped between 0 and 1.

	state.world.for_each_state_instance([&](dcon::state_instance_id si) {
		auto owner = state.world.state_instance_get_nation_from_state_ownership(si);

		auto admin_mod = state.world.nation_get_static_modifier_values(owner, sys::national_mod_offsets::administrative_efficiency - sys::provincial_mod_offsets::count) + state.world.nation_get_fluctuating_modifier_values(owner, sys::national_mod_offsets::administrative_efficiency - sys::provincial_mod_offsets::count);

		float issue_sum = 0.0f;
		for(auto i : state.culture_definitions.social_issues) {
			issue_sum = issue_sum + state.world.issue_option_get_administrative_multiplier(state.world.nation_get_issues(owner, i));
		}
		auto from_issues = issue_sum * state.defines.bureaucracy_percentage_increment + state.defines.max_bureaucracy_percentage;
		float non_core_effect = 0.0f;
		float bsum = 0.0f;
		for_each_province_in_state_instance(state, si, [&](dcon::province_id p) {
			if(!state.world.province_get_is_owner_core(p)) {
				non_core_effect += state.defines.noncore_tax_penalty;
			}
			for(auto po : state.world.province_get_pop_location(p)) {
				if(po.get_pop().get_is_primary_or_accepted_culture()) {
					bsum += po.get_pop().get_size();
				}
			}
		});
		auto total_pop = state.world.state_instance_get_demographics(si, demographics::total);
		auto total = total_pop > 0 ? std::clamp((non_core_effect + state.culture_definitions.bureaucrat_tax_efficiency * bsum / total_pop) / from_issues, 0.0f, 1.0f) : 0.0f;

		state.world.state_instance_set_administrative_efficiency(si, total);

	});
}
*/
bool has_railroads_being_built(sys::state& state, dcon::province_id id) {
	for(auto pb : state.world.province_get_province_building_construction(id)) {
		if(economy::province_building_type(pb.get_type()) == economy::province_building_type::railroad)
			return true;
	}
	return false;
}
bool can_build_railroads(sys::state& state, dcon::province_id id) {
	auto nation = state.world.province_get_nation_from_province_ownership(id);
	int32_t current_rails_lvl = state.world.province_get_railroad_level(id);
	int32_t max_local_rails_lvl = state.world.nation_get_max_railroad_level(nation);
	int32_t min_build_railroad = int32_t(state.world.province_get_modifier_values(id, sys::provincial_mod_offsets::min_build_railroad));

	return (max_local_rails_lvl - current_rails_lvl - min_build_railroad > 0) && !has_railroads_being_built(state, id);
}
bool has_an_owner(sys::state& state, dcon::province_id id) {
	// TODO: not sure if this is the most efficient way
	return bool(dcon::fatten(state.world, id).get_nation_from_province_ownership());
}
float monthly_net_pop_growth(sys::state& state, dcon::province_id id) {
	auto nation = state.world.province_get_nation_from_province_ownership(id);
	float total_pops = state.world.province_get_demographics(id, demographics::total);


	float life_rating = state.world.province_get_life_rating(id)
						* (1 + state.world.province_get_modifier_values(id, sys::provincial_mod_offsets::life_rating));
	life_rating = std::clamp(life_rating, 0.f, 40.f);

	if(life_rating > state.defines.min_life_rating_for_growth) {
		life_rating -= state.defines.min_life_rating_for_growth;
		life_rating *= state.defines.life_rating_growth_bonus;
	} else {
		life_rating = 0;
	}

	float growth_factor = life_rating + state.defines.base_popgrowth;

	float life_needs = state.world.province_get_demographics(id, demographics::poor_everyday_needs)
		+ state.world.province_get_demographics(id, demographics::middle_everyday_needs)
		+ state.world.province_get_demographics(id, demographics::rich_everyday_needs);

	life_needs /= total_pops;

	

	float growth_modifier_sum = ((life_needs - state.defines.life_need_starvation_limit) * growth_factor * 4
		+ state.world.province_get_modifier_values(id, sys::provincial_mod_offsets::population_growth)
		+ state.world.nation_get_modifier_values(nation, sys::national_mod_offsets::pop_growth) * 0.1f
		); // /state.defines.slave_growth_divisor;

	// TODO: slaves growth

	return growth_modifier_sum * total_pops;
}
float monthly_net_pop_promotion_and_demotion(sys::state& state, dcon::province_id id) {
	// TODO
	return 0.0f;
}
float monthly_net_pop_internal_migration(sys::state& state, dcon::province_id id) {
	// TODO
	return 0.0f;
}
float monthly_net_pop_external_migration(sys::state& state, dcon::province_id id) {
	// TODO
	return 0.0f;
}
float rgo_maximum_employment(sys::state& state, dcon::province_id id) {
	return economy::rgo_max_employment(state, state.world.province_get_nation_from_province_ownership(id), id);
}
float rgo_employment(sys::state& state, dcon::province_id id) {
	return economy::rgo_max_employment(state, state.world.province_get_nation_from_province_ownership(id), id) * state.world.province_get_rgo_employment(id);
}
float rgo_income(sys::state& state, dcon::province_id id) {
	return state.world.province_get_rgo_full_profit(id);
}
float rgo_production_quantity(sys::state& state, dcon::province_id id) {
	return economy::rgo_full_production_quantity(state, state.world.province_get_nation_from_province_ownership(id), id) * state.world.province_get_rgo_employment(id);
}
float rgo_size(sys::state& state, dcon::province_id prov_id) {
	bool is_mine = state.world.commodity_get_is_mine(state.world.province_get_rgo(prov_id));
	auto sz = state.world.province_get_rgo_size(prov_id);


	auto n = dcon::fatten(state.world, prov_id).get_nation_from_province_ownership();
	auto bonus = state.world.province_get_modifier_values(prov_id, is_mine ? sys::provincial_mod_offsets::mine_rgo_size : sys::provincial_mod_offsets::farm_rgo_size)
		+ state.world.nation_get_modifier_values(n, is_mine ? sys::national_mod_offsets::mine_rgo_size : sys::national_mod_offsets::farm_rgo_size)
		+ state.world.nation_get_rgo_size(n, state.world.province_get_rgo(prov_id))
		+ 1.0f;
	return sz * bonus;
}

float state_accepted_bureaucrat_size(sys::state& state, dcon::state_instance_id id) {
	float bsum = 0.f;
	for_each_province_in_state_instance(state, id, [&](dcon::province_id p) {
		for(auto po : state.world.province_get_pop_location(p)) {
			if(po.get_pop().get_is_primary_or_accepted_culture() && po.get_pop().get_poptype() == state.culture_definitions.bureaucrat) {
				bsum += po.get_pop().get_size();
			}
		}
	});
	return bsum;
}

float state_admin_efficiency(sys::state& state, dcon::state_instance_id id) {
	auto owner = state.world.state_instance_get_nation_from_state_ownership(id);

	auto admin_mod = state.world.nation_get_modifier_values(owner, sys::national_mod_offsets::administrative_efficiency_modifier);

	float issue_sum = 0.0f;
	for(auto i : state.culture_definitions.social_issues) {
		issue_sum += state.world.issue_option_get_administrative_multiplier(state.world.nation_get_issues(owner, i));
	}
	auto from_issues = issue_sum * state.defines.bureaucracy_percentage_increment + state.defines.max_bureaucracy_percentage;
	float non_core_effect = 0.0f;
	float bsum = 0.0f;
	for_each_province_in_state_instance(state, id, [&](dcon::province_id p) {
		if(!state.world.province_get_is_owner_core(p)) {
			non_core_effect += state.defines.noncore_tax_penalty;
		}
		for(auto po : state.world.province_get_pop_location(p)) {
			if(po.get_pop().get_is_primary_or_accepted_culture() && po.get_pop().get_poptype() == state.culture_definitions.bureaucrat) {
				bsum += po.get_pop().get_size();
			}
		}
	});
	auto total_pop = state.world.state_instance_get_demographics(id, demographics::total);
	auto total = total_pop > 0 ? std::clamp(
		admin_mod +
		non_core_effect +
		state.defines.base_country_admin_efficiency +
		std::min(state.culture_definitions.bureaucrat_tax_efficiency * bsum / total_pop, 1.0f) / from_issues, 0.0f, 1.0f) : 0.0f;

	return total;
}
float crime_fighting_efficiency(sys::state& state, dcon::province_id id) {
	// TODO
	/*
	Crime is apparently the single place where the following value matters:
	- state administrative efficiency: = define:NONCORE_TAX_PENALTY x number-of-non-core-provinces + (bureaucrat-tax-efficiency x total-number-of-primary-or-accepted-culture-bureaucrats / population-of-state)v1 / x (sum-of-the-administrative_multiplier-for-social-issues-marked-as-being-administrative x define:BUREAUCRACY_PERCENTAGE_INCREMENT + define:MAX_BUREAUCRACY_PERCENTAGE)), all clamped between 0 and 1.
	The crime fighting percent of a province is then calculated as: (state-administration-efficiency x define:ADMIN_EFFICIENCY_CRIMEFIGHT_PERCENT + administration-spending-setting x (1 - ADMIN_EFFICIENCY_CRIMEFIGHT_PERCENT)) x (define:MAX_CRIMEFIGHTING_PERCENT - define:MIN_CRIMEFIGHTING_PERCENT) + define:MIN_CRIMEFIGHTING_PERCENT
	*/
	// we have agreed to replace admin spending with national admin efficiency

	auto si = state.world.province_get_state_membership(id);
	auto owner = state.world.province_get_nation_from_province_ownership(id);
	if(si && owner)
		return (state_admin_efficiency(state, si) * state.defines.admin_efficiency_crimefight_percent + (1 - state.defines.admin_efficiency_crimefight_percent) * state.world.nation_get_administrative_efficiency(owner)) * (state.defines.max_crimefight_percent - state.defines.min_crimefight_percent) + state.defines.min_crimefight_percent;
	else
		return 0.0f;
}
float revolt_risk(sys::state& state, dcon::province_id id) {
	auto total_pop = state.world.province_get_demographics(id, demographics::total);
	if(total_pop == 0) {
		return 0;
	}

	auto militancy = state.world.province_get_demographics(id, demographics::militancy);
	return militancy / total_pop;
}

dcon::province_id get_connected_province(sys::state& state, dcon::province_adjacency_id adj, dcon::province_id curr) {
	auto first = state.world.province_adjacency_get_connected_provinces(adj, 0);
	if(first == curr) {
		return state.world.province_adjacency_get_connected_provinces(adj, 1);
	} else {
		return first;
	}
}

struct queue_node {
	float priority;
	dcon::province_id prov_id;
};

float state_distance(sys::state& state, dcon::state_instance_id state_id, dcon::province_id prov_id) {
	// TODO
	return 1.f;
}

bool can_integrate_colony(sys::state& state, dcon::state_instance_id id) {
	auto dkey = demographics::to_key(state, state.culture_definitions.bureaucrat);
	auto bureaucrat_size = state_accepted_bureaucrat_size(state, id);
	auto total_size = state.world.state_instance_get_demographics(id, demographics::total);
	if(bureaucrat_size / total_size >= state.defines.state_creation_admin_limit) {
		auto owner = state.world.state_instance_get_nation_from_state_ownership(id);
		return colony_integration_cost(state, id) <= nations::free_colonial_points(state, owner);
	} else {
		return false;
	}
}

float colony_integration_cost(sys::state& state, dcon::state_instance_id id) {
	bool entirely_overseas = true;
	float prov_count = 0.f;
	for_each_province_in_state_instance(state, id, [&](dcon::province_id prov) {
		entirely_overseas &= is_overseas(state, prov);
		prov_count++;
	});
	if(entirely_overseas) {
		auto owner = state.world.state_instance_get_nation_from_state_ownership(id);
		float distance = state_distance(state, id, state.world.nation_get_capital(owner).id);
		return state.defines.colonization_create_state_cost * prov_count * std::max(distance / state.defines.colonization_colony_state_distance, 1.f);
	} else {
		return 0.f;
	}
}

void change_province_owner(sys::state& state, dcon::province_id id, dcon::nation_id new_owner) {
	state.adjacency_data_out_of_date = true;
	state.national_cached_values_out_of_date = true;

	auto state_def = state.world.province_get_state_from_abstract_state_membership(id);
	auto old_si = state.world.province_get_state_membership(id);
	auto old_owner = state.world.province_get_nation_from_province_ownership(id);

	if(new_owner) {
		dcon::state_instance_id new_si;
		for(auto si : state.world.nation_get_state_ownership(new_owner)) {
			if(si.get_state().get_definition().id == state_def) {
				new_si = si.get_state().id;
				break;
			}
		}
		if(!new_si) {
			new_si = state.world.create_state_instance();
			state.world.state_instance_set_definition(new_si, state_def);
			state.world.try_create_state_ownership(new_si, new_owner);
		}
		int32_t factories_in_new_state = 0;
		province::for_each_province_in_state_instance(state, new_si, [&](dcon::province_id pr) {
			auto fac_range = state.world.province_get_factory_location(pr);
			factories_in_new_state += int32_t(fac_range.end() - fac_range.begin());
		});

		auto province_fac_range = state.world.province_get_factory_location(id);
		int32_t factories_in_province = int32_t(province_fac_range.end() - province_fac_range.begin());

		auto excess_factories = std::min((factories_in_new_state + factories_in_province) - 8, factories_in_province);
		if(excess_factories > 0) {
			std::vector<dcon::factory_id> to_delete;
			while(excess_factories > 0) {
				to_delete.push_back((*(province_fac_range.begin() + excess_factories)).get_factory().id);
				--excess_factories;
			}
			for(auto fid : to_delete) {
				state.world.delete_factory(fid);
			}
		}

		state.world.province_set_state_membership(id, new_si);

		for(auto p : state.world.province_get_pop_location(id)) {
			if(state.world.nation_get_primary_culture(new_owner) == p.get_pop().get_culture()) {
				p.get_pop().set_is_primary_or_accepted_culture(true);
				continue;
			}
			auto accepted = state.world.nation_get_accepted_cultures(new_owner);
			for(auto c : accepted) {
				if(c == p.get_pop().get_culture()) {
					p.get_pop().set_is_primary_or_accepted_culture(true);
					continue;
				}
			}
			p.get_pop().set_is_primary_or_accepted_culture(false);
		}
	} else {
		state.world.province_set_state_membership(id, dcon::state_instance_id{});
	}

	for(auto p : state.world.province_get_pop_location(id)) {
		rebel::remove_pop_from_movement(state, p.get_pop());
		rebel::remove_pop_from_rebel_faction(state, p.get_pop());

		std::vector<dcon::regiment_id> regs;
		for(auto r : p.get_pop().get_regiment_source()) {
			regs.push_back(r.get_regiment().id);
		}
		for(auto r : regs) {
			state.world.delete_regiment(r);
		}
	}

	state.world.province_set_nation_from_province_ownership(id, new_owner);
	state.world.province_set_last_control_change(id, state.current_date);
	state.world.province_set_nation_from_province_control(id, new_owner);
	state.world.province_set_is_owner_core(id, bool(state.world.get_core_by_prov_tag_key(id, state.world.nation_get_identity_from_identity_holder(new_owner))));

	if(old_si) {
		int32_t provinces_in_old_si = 0;
		province::for_each_province_in_state_instance(state, old_si, [&](auto) { ++provinces_in_old_si; });
		if(provinces_in_old_si == 0) {
			if(old_si == state.crisis_state)
				nations::cleanup_crisis(state);
			state.world.delete_state_instance(old_si);
		}
	}

	if(old_owner) {
		auto old_owner_rem_provs = state.world.nation_get_province_ownership(old_owner);
		if(old_owner_rem_provs.begin() == old_owner_rem_provs.end()) {
			nations::cleanup_nation(state, old_owner);
		}
	}

	// cancel constructions

	{
		auto rng = state.world.province_get_province_building_construction(id);
		while(rng.begin() != rng.end()) {
			state.world.delete_province_building_construction(*(rng.begin()));
		}
	}

	{
		auto rng = state.world.province_get_province_land_construction(id);
		while(rng.begin() != rng.end()) {
			state.world.delete_province_land_construction(*(rng.begin()));
		}
	}

	{
		auto rng = state.world.province_get_province_naval_construction(id);
		while(rng.begin() != rng.end()) {
			state.world.delete_province_naval_construction(*(rng.begin()));
		}
	}
}

void update_crimes(sys::state& state) {
	for_each_land_province(state, [&](dcon::province_id p) {
		auto owner = state.world.province_get_nation_from_province_ownership(p);
		if(!owner)
			return;

		/*
		Once per month (the 1st) province crimes are updated. If the province has a crime, the crime fighting percent is the probability of that crime being removed. If there is no crime, the crime fighting percent is the probability that it will remain crime free. If a crime is added to the province, it is selected randomly (with equal probability) from the crimes that are possible for the province (determined by the crime being activated and its trigger passing).
		*/

		auto chance = uint32_t(province::crime_fighting_efficiency(state, p) * 256.0f);
		auto rvalues = rng::get_random_pair(state, uint32_t((p.index() << 2) + 1));
		if((rvalues .high & 0xFF) >= chance) {
			state.world.province_set_crime(p, dcon::crime_id{});
		} else {
			if(!state.world.province_get_crime(p)) {
				static std::vector<dcon::crime_id> possible_crimes;
				possible_crimes.clear();

				for(uint32_t i = 0; i < state.culture_definitions.crimes.size(); ++i) {
					dcon::crime_id c{dcon::crime_id::value_base_t(i) };
					if(state.culture_definitions.crimes[c].available_by_default || state.world.nation_get_active_crime(owner, c)) {
						if(auto t = state.culture_definitions.crimes[c].trigger; t) {
							if(trigger::evaluate(state, t, trigger::to_generic(p), trigger::to_generic(owner), 0))
								possible_crimes.push_back(c);
						} else {
							possible_crimes.push_back(c);
						}
					}
				}

				if(auto count = possible_crimes.size(); count != 0) {
					auto selected = possible_crimes[rvalues.low % count];
					state.world.province_set_crime(p, selected);
				}
				
			}
		}
	});
}

bool can_start_colony(sys::state& state, dcon::nation_id n, dcon::state_definition_id d) {
	if(state.world.state_definition_get_colonization_stage(d) > uint8_t(1))
		return false; // too late

	// Your country must be of define:COLONIAL_RANK or less.
	if(state.world.nation_get_rank(n) > uint16_t(state.defines.colonial_rank))
		return false; // too low rank to colonize;

	// The state may not be the current target of a crisis, nor may your country be involved in an active crisis war.
	if(state.crisis_colony == d)
		return false;
	for(auto par : state.world.war_get_war_participant(state.crisis_war)) {
		if(par.get_nation() == n)
			return false;
	}

	float max_life_rating = -1.0f;
	for(auto p : state.world.state_definition_get_abstract_state_membership(d)) {
		if(!p.get_province().get_nation_from_province_ownership()) {
			max_life_rating = std::max(max_life_rating, float(p.get_province().get_life_rating()));
		}
	}

	if(max_life_rating < 0.0f) {
		return false; // no uncolonized province
	}

	/*
	You must have colonial life rating points from technology + define:COLONIAL_LIFERATING less than or equal to the *greatest* life rating of an unowned province in the state
	*/

	if(state.defines.colonial_liferating + state.world.nation_get_modifier_values(n, sys::national_mod_offsets::colonial_life_rating) > max_life_rating) {
		return false;
	}

	auto colonizers = state.world.state_definition_get_colonization(d);
	auto num_colonizers = colonizers.end() - colonizers.begin();

	// You can invest colonially in a region if there are fewer than 4 other colonists there (or you already have a colonist there)
	if(num_colonizers >= 4)
		return false;

	for(auto c : colonizers) {
		if(c.get_colonizer() == n)
			return false; // already started a colony
	}

	/*
	If you haven't yet put a colonist into the region, you must be in range of the region. Any region adjacent to your country or to one of your vassals or substates is considered to be in range. Otherwise it must be in range of one of your naval bases, with the range depending on the colonial range value provided by the naval base building x the level of the naval base.
	*/
	bool nation_has_port = state.world.nation_get_central_ports(n) != 0;
	bool adjacent_or_coastal = [&]() {
		for(auto p : state.world.state_definition_get_abstract_state_membership(d)) {
			if(!p.get_province().get_nation_from_province_ownership()) {
				if(nation_has_port && p.get_province().get_is_coast())
					return true;

				for(auto adj : p.get_province().get_province_adjacency()) {
					auto indx = adj.get_connected_provinces(0) != p.get_province() ? 0 : 1;
					auto o = adj.get_connected_provinces(indx).get_nation_from_province_ownership();
					if(o == n)
						return true;
					if(o.get_overlord_as_subject().get_ruler() == n)
						return true;
				}
			}
		}
		return false;
	}();

	if(!adjacent_or_coastal)
		return false;


	return true;
}

void update_colonization(sys::state& state) {
	for(auto d : state.world.in_state_definition) {
		auto colonizers = state.world.state_definition_get_colonization(d);
		auto num_colonizers = colonizers.end() - colonizers.begin();

		if(num_colonizers == 0 && d.get_colonization_stage() != 0) {
			d.set_colonization_stage(uint8_t(0));
		} else if(num_colonizers > 1 && d.get_colonization_stage() == uint8_t(2)) {
			/*
			In phase 2 if there are competing colonizers, the "temperature" in the colony will rise by define:COLONIAL_INFLUENCE_TEMP_PER_DAY + maximum-points-invested x define:COLONIAL_INFLUENCE_TEMP_PER_LEVEL + define:TENSION_WHILE_CRISIS (if there is some other crisis going on) + define:AT_WAR_TENSION_DECAY (if either of the two colonizers are at war or disarmed)
			*/

			int32_t max_points = 0;
			float at_war_adjust = 0.0f;
			for(auto c : colonizers) {
				max_points = std::max(max_points, int32_t(c.get_level()));
				if(state.world.nation_get_is_at_war(c.get_colonizer()) || (state.world.nation_get_disarmed_until(c.get_colonizer()) && state.current_date <= state.world.nation_get_disarmed_until(c.get_colonizer()))) {
					at_war_adjust = state.defines.at_war_tension_decay;
				}
			}

			float adjust = state.defines.colonization_influence_temperature_per_day + float(max_points) * state.defines.colonization_influence_temperature_per_level + (state.current_crisis != sys::crisis_type::none ? state.defines.tension_while_crisis : 0.0f) + at_war_adjust;

			d.set_colonization_temperature(std::clamp(d.get_colonization_temperature() + adjust, 0.0f, 100.0f));
		} else if(num_colonizers == 1 && (*colonizers.begin()).get_last_investment() + int32_t(state.defines.colonization_days_for_initial_investment) <= state.current_date) {
			/*
			If you have put in a colonist in a region and it goes at least define:COLONIZATION_DAYS_FOR_INITIAL_INVESTMENT without any other colonizers, it then moves into phase 3 with define:COLONIZATION_INTEREST_LEAD points.
			*/

			d.set_colonization_stage(uint8_t(3));
		} else if(d.get_colonization_stage() == uint8_t(3) && num_colonizers != 0) {
			/*
			If you leave a colony in phase 3 for define:COLONIZATION_MONTHS_TO_COLONIZE months, the colonization will reset to phase 0 (no colonization in progress).
			*/
			if((*colonizers.begin()).get_last_investment() + 31 * int32_t(state.defines.colonization_months_to_colonize) <= state.current_date) {

				d.set_colonization_stage(uint8_t(0));
				do {
					state.world.delete_colonization(*(colonizers.begin()));
				} while(colonizers.end() != colonizers.begin());
			}
		}
	}
}

bool state_is_coastal(sys::state& state, dcon::state_instance_id s) {
	auto d = state.world.state_instance_get_definition(s);
	auto o = state.world.state_instance_get_nation_from_state_ownership(s);
	for(auto p : state.world.state_definition_get_abstract_state_membership(d)) {
		if(p.get_province().get_nation_from_province_ownership() == o) {
			if(p.get_province().get_is_coast())
				return true;
		}
	}
	return false;
}

}

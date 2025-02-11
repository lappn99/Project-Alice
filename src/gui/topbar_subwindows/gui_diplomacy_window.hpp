#pragma once

#include "dcon_generated.hpp"
#include "gui_element_types.hpp"
#include "gui_graphics.hpp"
#include "text.hpp"
#include "parsers.hpp"
#include <algorithm>
#include <functional>

#include "gui_diplomacy_actions_window.hpp"

namespace ui {

enum class diplomacy_window_tab : uint8_t {
	great_powers = 0x0,
	wars = 0x1,
	casus_belli = 0x2,
	crisis = 0x3
};

class diplomacy_country_select : public button_element_base {
public:
	message_result on_scroll(sys::state& state, int32_t x, int32_t y, float amount, sys::key_modifiers mods) noexcept override {
		return parent->impl_on_scroll(state, x, y, amount, mods);
	}

	void button_action(sys::state& state) noexcept override {
		Cyto::Any payload = button_press_notification{};
		parent->impl_get(state, payload);
	}
};

class diplomacy_country_info : public listbox_row_element_base<dcon::nation_id> {
private:
	flag_button* country_flag = nullptr;
public:
	void on_create(sys::state& state) noexcept override {
		listbox_row_element_base<dcon::nation_id>::on_create(state);
		base_data.position.x -= 14;
		base_data.position.y -= 524;
	}

	std::unique_ptr<element_base> make_child(sys::state& state, std::string_view name, dcon::gui_def_id id) noexcept override {
		if(name == "country_select") {
			return make_element_by_type<diplomacy_country_select>(state, id);
		} else if(name == "country_flag") {
			auto ptr = make_element_by_type<flag_button>(state, id);
			ptr->base_data.position.y -= 2; // Nudge
			country_flag = ptr.get();
			return ptr;
		} else if(name == "country_name") {
			return make_element_by_type<generic_name_text<dcon::nation_id>>(state, id);
		} else if(name == "country_boss_flag") {
			return make_element_by_type<nation_overlord_flag>(state, id);
		} else if(name == "country_prestige") {
			return make_element_by_type<nation_prestige_rank_text>(state, id);
		} else if(name == "country_economic") {
			return make_element_by_type<nation_industry_rank_text>(state, id);
		} else if(name == "country_military") {
			return make_element_by_type<nation_military_rank_text>(state, id);
		} else if(name == "country_total") {
			return make_element_by_type<nation_rank_text>(state, id);
		} else if(name == "country_opinion") {
			return make_element_by_type<nation_player_opinion_text>(state, id);
		} else if(name == "country_relation") {
			return make_element_by_type<nation_player_relations_text>(state, id);
		} else if(name.substr(0, 10) == "country_gp") {
			auto ptr = make_element_by_type<nation_gp_opinion_text>(state, id);
			ptr->rank = uint16_t(std::stoi(std::string{ name.substr(10) }));
			return ptr;
		} else {
			return nullptr;
		}
	}

	void update(sys::state& state) noexcept override {
		country_flag->on_update(state);
		Cyto::Any payload = content;
		impl_set(state, payload);
	}

	message_result get(sys::state& state, Cyto::Any& payload) noexcept override {
		if(payload.holds_type<dcon::nation_id>()) {
			payload.emplace<dcon::nation_id>(content);
			return message_result::consumed;
		} else if(payload.holds_type<button_press_notification>()) {
			Cyto::Any new_payload = content;
			return parent->impl_get(state, new_payload);
		} else {
			return listbox_row_element_base<dcon::nation_id>::get(state, payload);
		}
	}
};

class diplomacy_country_listbox : public listbox_element_base<diplomacy_country_info, dcon::nation_id> {
protected:
	std::string_view get_row_element_name() override {
        return "diplomacy_country_info";
    }
};

class diplomacy_country_facts : public window_element_base {
private:
	dcon::nation_id active_nation{};
	flag_button* country_flag = nullptr;
	nation_player_relations_text* country_relation = nullptr;
	image_element_base* country_relation_icon = nullptr;
	simple_text_element_base* country_primary_cultures = nullptr;
	simple_text_element_base* country_accepted_cultures = nullptr;
public:
	void on_create(sys::state& state) noexcept override {
		window_element_base::on_create(state);
		if(bool(state.local_player_nation)) {
			Cyto::Any payload = state.local_player_nation;
			set(state, payload);
		}
	}

	std::unique_ptr<element_base> make_child(sys::state& state, std::string_view name, dcon::gui_def_id id) noexcept override {
		if(name == "country_flag") {
			auto ptr = make_element_by_type<flag_button>(state, id);
			country_flag = ptr.get();
			return ptr;
		} else if(name == "country_flag_overlay") {
			return make_element_by_type<nation_flag_frame>(state, id);
		} else if(name == "country_name") {
			return make_element_by_type<generic_name_text<dcon::nation_id>>(state, id);
		} else if(name == "country_status") {
			return make_element_by_type<nation_status_text>(state, id);
		} else if(name == "selected_nation_totalrank") {
			return make_element_by_type<nation_rank_text>(state, id);
		} else if(name == "ideology_icon") {
			return make_element_by_type<nation_ruling_party_ideology_plupp>(state, id);
		} else if(name == "country_party") {
			return make_element_by_type<nation_ruling_party_text>(state, id);
		} else if(name == "country_gov") {
			return make_element_by_type<nation_government_type_text>(state, id);
		} else if(name == "country_tech") {
			return make_element_by_type<nation_technology_admin_type_text>(state, id);
		} else if(name == "our_relation_icon") {
			auto ptr = make_element_by_type<image_element_base>(state, id);
			country_relation_icon = ptr.get();
			return ptr;
		} else if(name == "our_relation") {
			auto ptr = make_element_by_type<nation_player_relations_text>(state, id);
			country_relation = ptr.get();
			return ptr;
		} else if(name == "country_prestige") {
			return make_element_by_type<nation_prestige_text>(state, id);
		} else if(name == "selected_prestige_rank") {
			return make_element_by_type<nation_prestige_rank_text>(state, id);
		} else if(name == "country_economic") {
			return make_element_by_type<nation_industry_score_text>(state, id);
		} else if(name == "selected_industry_rank") {
			return make_element_by_type<nation_industry_rank_text>(state, id);
		} else if(name == "country_military") {
			return make_element_by_type<nation_military_score_text>(state, id);
		} else if(name == "selected_military_rank") {
			return make_element_by_type<nation_military_rank_text>(state, id);
		} else if(name == "country_total") {
			return make_element_by_type<nation_total_score_text>(state, id);
		} else if(name == "selected_total_rank") {
			return make_element_by_type<nation_rank_text>(state, id);
		} else if(name == "country_population") {
			return make_element_by_type<nation_population_text>(state, id);
		} else if(name == "country_primary_cultures") {
			auto ptr = make_element_by_type<simple_text_element_base>(state, id);
			country_primary_cultures = ptr.get();
			return ptr;
		} else if(name == "country_accepted_cultures") {
			auto ptr = make_element_by_type<simple_text_element_base>(state, id);
			country_accepted_cultures = ptr.get();
			return ptr;
		} else if(name == "country_wars") {
			auto ptr = make_element_by_type<overlapping_enemy_flags>(state, id);
			ptr->base_data.position.y -= 8;
			return ptr;
		} else if(name == "country_allies") {
			auto ptr = make_element_by_type<overlapping_ally_flags>(state, id);
			ptr->base_data.position.y -= 8;
			return ptr;
		} else if(name == "country_protected") {
			auto ptr = make_element_by_type<overlapping_protected_flags>(state, id);
			ptr->base_data.position.y -= 8;
			return ptr;
		} else if(name == "country_truce") {
			auto ptr = make_element_by_type<overlapping_truce_flags>(state, id);
			ptr->base_data.position.y -= 8;
			return ptr;
		} else if(name == "infamy_text") {
			return make_element_by_type<nation_infamy_text>(state, id);
		} else if(name == "warexhastion_text") {
			return make_element_by_type<nation_war_exhaustion_text>(state, id);
		} else if(name == "brigade_text") {
			return make_element_by_type<nation_brigades_text>(state, id);
		} else if(name == "ships_text") {
			return make_element_by_type<nation_ships_text>(state, id);
		} else {
			return nullptr;
		}
	}

	void on_update(sys::state& state) noexcept override {
		auto fat_id = dcon::fatten(state.world, active_nation);

		{
			const auto culture_id = fat_id.get_primary_culture();
			auto culture = dcon::fatten(state.world, culture_id);
			const auto text = text::produce_simple_string(state, culture.get_name());
			country_primary_cultures->set_text(state, text);
		}

		{
			std::string text{};
			for(const auto culture_id : fat_id.get_accepted_cultures()) {
				auto culture = dcon::fatten(state.world, culture_id);
				text += text::produce_simple_string(state, culture.get_name()) + ", ";
			}
			country_accepted_cultures->set_text(state, "");
		}
	}

	message_result get(sys::state& state, Cyto::Any& payload) noexcept override {
		if(payload.holds_type<dcon::nation_id>()) {
			payload.emplace<dcon::nation_id>(active_nation);
			return message_result::consumed;
		} else {
			return message_result::unseen;
		}
	}

	message_result set(sys::state& state, Cyto::Any& payload) noexcept override {
		if(payload.holds_type<dcon::nation_id>()) {
			active_nation = any_cast<dcon::nation_id>(payload);
			country_flag->on_update(state);

			country_relation->set_visible(state, active_nation != state.local_player_nation);
			country_relation_icon->set_visible(state, active_nation != state.local_player_nation);
			
			on_update(state);
			return message_result::consumed;
		} else {
			return message_result::unseen;
		}
	}
};

class overlapping_attacker_flags : public overlapping_flags_box {
protected:
	void populate_flags(sys::state& state) override {
		row_contents.clear();
		auto war = dcon::fatten(state.world, war_id);
		for(auto o : war.get_war_participant())
			if(o.get_is_attacker() == true)
				row_contents.push_back(o.get_nation().get_identity_from_identity_holder().id);
		update(state);
	}
	dcon::war_id war_id{};
public:
	message_result set(sys::state& state, Cyto::Any& payload) noexcept override {
		if(payload.holds_type<dcon::war_id>()) {
			war_id = any_cast<dcon::war_id>(payload);
			on_update(state);
			return message_result::consumed;
		}
		return overlapping_flags_box::set(state, payload);
	}
};
class overlapping_defender_flags : public overlapping_flags_box {
protected:
	void populate_flags(sys::state& state) override {
		row_contents.clear();
		auto war = dcon::fatten(state.world, war_id);
		for(auto o : war.get_war_participant())
			if(o.get_is_attacker() == false)
				row_contents.push_back(o.get_nation().get_identity_from_identity_holder().id);
		update(state);
	}
	dcon::war_id war_id{};
public:
	message_result set(sys::state& state, Cyto::Any& payload) noexcept override {
		if(payload.holds_type<dcon::war_id>()) {
			war_id = any_cast<dcon::war_id>(payload);
			on_update(state);
			return message_result::consumed;
		}
		return overlapping_flags_box::set(state, payload);
	}
};

template<bool IsAttacker>
class war_side_strength_text : public generic_settable_element<button_element_base, dcon::war_id> {
public:
	void on_update(sys::state& state) noexcept override {
		auto fat_id = dcon::fatten(state.world, content);
		int32_t strength = 0;
		for(auto o : fat_id.get_war_participant())
			if(o.get_is_attacker() == IsAttacker)
				strength += int32_t(o.get_nation().get_military_score());
		set_button_text(state, std::to_string(strength));
	}

	tooltip_behavior has_tooltip(sys::state& state) noexcept override {
		return tooltip_behavior::variable_tooltip;
	}

	void update_tooltip(sys::state& state, int32_t x, int32_t y, text::columnar_layout& contents) noexcept override {
		auto fat_id = dcon::fatten(state.world, content);
		for(auto o : fat_id.get_war_participant())
			if(o.get_is_attacker() == IsAttacker) {
				auto name = o.get_nation().get_name();
				auto box = text::open_layout_box(contents, 0);
				text::add_to_layout_box(contents, state, box, text::produce_simple_string(state, name), text::text_color::yellow);
				text::add_to_layout_box(contents, state, box, std::string{ ":" }, text::text_color::yellow);
				text::add_space_to_layout_box(contents, state, box);
				auto strength = int32_t(o.get_nation().get_military_score());
				text::add_to_layout_box(contents, state, box, std::to_string(strength), text::text_color::white);
				text::close_layout_box(contents, box);
			}
	}
};

class diplomacy_war_info : public listbox_row_element_base<dcon::war_id> {
	simple_text_element_base* war_name = nullptr;
	simple_text_element_base* attackers_strength_text = nullptr;
	simple_text_element_base* defenders_strength_text = nullptr;
	overlapping_attacker_flags* attackers_flags = nullptr;
	overlapping_defender_flags* defenders_flags = nullptr;
public:
	void on_create(sys::state& state) noexcept override {
		listbox_row_element_base::on_create(state);
		base_data.position.x = base_data.position.y = 0;
	}

	std::unique_ptr<element_base> make_child(sys::state& state, std::string_view name, dcon::gui_def_id id) noexcept override {
		if(name == "diplo_war_entrybg") {
			return make_element_by_type<image_element_base>(state, id);
		} else if(name == "war_name") {
			auto ptr = make_element_by_type<simple_text_element_base>(state, id);
			war_name = ptr.get();
			return ptr;
		} else if(name == "attackers_mil_strength") {
			auto ptr = make_element_by_type<war_side_strength_text<true>>(state, id);
			ptr->base_data.position.y -= 4; // Nudge
			return ptr;
		} else if(name == "defenders_mil_strength") {
			auto ptr = make_element_by_type<war_side_strength_text<false>>(state, id);
			ptr->base_data.position.y -= 4; // Nudge
			return ptr;
		} else if(name == "warscore") {
			return make_element_by_type<image_element_base>(state, id);
		} else if(name == "diplo_war_progress_overlay") {
			return make_element_by_type<image_element_base>(state, id);
		} else if(name == "diplo_warscore_marker1") {
			return make_element_by_type<image_element_base>(state, id);
		} else if(name == "diplo_warscore_marker2") {
			return make_element_by_type<image_element_base>(state, id);
		} else if(name == "warscore_text") {
			return make_element_by_type<simple_text_element_base>(state, id);
		} else if(name == "attackers") {
			auto ptr = make_element_by_type<overlapping_attacker_flags>(state, id);
			attackers_flags = ptr.get();
			attackers_flags->base_data.position.y -= 8 - 2;
			return ptr;
		} else if(name == "defenders") {
			auto ptr = make_element_by_type<overlapping_defender_flags>(state, id);
			defenders_flags = ptr.get();
			defenders_flags->base_data.position.y -= 8 - 2;
			return ptr;
		} else if(name == "join_attackers") {
			return make_element_by_type<button_element_base>(state, id);
		} else if(name == "join_defenders") {
			return make_element_by_type<button_element_base>(state, id);
		} else {
			return nullptr;
		}
	}

	void update(sys::state& state) noexcept override {
		Cyto::Any payload = content;
		impl_set(state, payload);

		auto war = dcon::fatten(state.world, content);
		war_name->set_text(state, text::produce_simple_string(state, war.get_name()));
	}
};

class diplomacy_war_listbox : public listbox_element_base<diplomacy_war_info, dcon::war_id> {
protected:
	std::string_view get_row_element_name() override {
        return "diplomacy_war_info";
    }
public:
	void on_update(sys::state& state) noexcept override {
		row_contents.clear();
		state.world.for_each_war([&](dcon::war_id id) {
			row_contents.push_back(id);
		});
		update(state);
	}
};

class diplomacy_greatpower_info : public window_element_base {
public:
	uint8_t rank = 0;

	std::unique_ptr<element_base> make_child(sys::state& state, std::string_view name, dcon::gui_def_id id) noexcept override {
		if(name == "country_name") {
			return make_element_by_type<generic_name_text<dcon::nation_id>>(state, id);
		} else if(name == "country_flag") {
			auto ptr = make_element_by_type<flag_button>(state, id);
			//ptr->base_data.size.x += 3; // Nudge
			//ptr->base_data.size.y += 4;
			return ptr;
		} else if(name == "country_puppets") {
			return make_element_by_type<overlapping_protected_flags>(state, id);
		} else if(name == "gp_prestige") {
			return make_element_by_type<nation_prestige_text>(state, id);
		} else if(name == "gp_economic") {
			return make_element_by_type<nation_industry_score_text>(state, id);
		} else if(name == "gp_military") {
			return make_element_by_type<nation_military_score_text>(state, id);
		} else if(name == "gp_total") {
			return make_element_by_type<nation_total_score_text>(state, id);
		} else {
			return nullptr;
		}
	}

    message_result get(sys::state& state, Cyto::Any& payload) noexcept override {
        if(payload.holds_type<dcon::nation_id>()) {
            payload.emplace<dcon::nation_id>(nations::get_nth_great_power(state, rank));
            return message_result::consumed;
        }
        return window_element_base::get(state, payload);
    }

	void on_update(sys::state& state) noexcept override {
		Cyto::Any payload = nations::get_nth_great_power(state, rank);
		impl_set(state, payload);
	}
};

enum class diplomacy_list_sort : uint8_t {
	country, boss, economic_rank, military_rank, prestige_rank, total_rank, relation, opinion
};
template<diplomacy_list_sort Sort>
class diplomacy_sort_button : public button_element_base {
public:
	void button_action(sys::state& state) noexcept override {
		if(parent) {
			Cyto::Any payload = Sort;
			parent->impl_set(state, payload);
		}
	}
};

class diplomacy_window : public generic_tabbed_window<diplomacy_window_tab> {
private:
	diplomacy_country_listbox* country_listbox = nullptr;
	diplomacy_war_listbox* war_listbox = nullptr;
	diplomacy_country_facts* country_facts = nullptr;
	diplomacy_action_dialog_window* action_dialog_win = nullptr;
	diplomacy_gp_action_dialog_window* gp_action_dialog_win = nullptr;
	element_base* casus_belli_window = nullptr;
	element_base* crisis_window = nullptr;
	std::vector<diplomacy_greatpower_info*> gp_infos{};
	std::vector<element_base*> action_buttons{};

	diplomacy_list_sort sort = diplomacy_list_sort::country;
	bool sort_ascend = true;

	void sort_countries(sys::state& state) {
		std::function<bool(dcon::nation_id, dcon::nation_id)> fn;
		switch(sort) {
		case diplomacy_list_sort::country:
			fn = [&](dcon::nation_id a, dcon::nation_id b) {
				dcon::nation_fat_id a_fat_id = dcon::fatten(state.world, a);
				auto a_name = text::produce_simple_string(state, a_fat_id.get_name());
				dcon::nation_fat_id b_fat_id = dcon::fatten(state.world, b);
				auto b_name = text::produce_simple_string(state, b_fat_id.get_name());
				return a_name < b_name;
			};
			break;
		case diplomacy_list_sort::economic_rank:
			fn = [&](dcon::nation_id a, dcon::nation_id b) {
				return state.world.nation_get_industrial_rank(a) < state.world.nation_get_industrial_rank(b);
			};
			break;
		case diplomacy_list_sort::military_rank:
			fn = [&](dcon::nation_id a, dcon::nation_id b) {
				return state.world.nation_get_military_rank(a) < state.world.nation_get_military_rank(b);
			};
			break;
		case diplomacy_list_sort::prestige_rank:
			fn = [&](dcon::nation_id a, dcon::nation_id b) {
				return state.world.nation_get_prestige_rank(a) < state.world.nation_get_prestige_rank(b);
			};
			break;
		case diplomacy_list_sort::total_rank:
			fn = [&](dcon::nation_id a, dcon::nation_id b) {
				return state.world.nation_get_rank(a) < state.world.nation_get_rank(b);
			};
			break;
		case diplomacy_list_sort::relation:
			fn = [&](dcon::nation_id a, dcon::nation_id b) {
				auto rid_a = state.world.get_diplomatic_relation_by_diplomatic_pair(state.local_player_nation, a);
				auto rid_b = state.world.get_diplomatic_relation_by_diplomatic_pair(state.local_player_nation, b);
				if(!bool(rid_a) || !bool(rid_b))
					return a.index() < b.index();
				return state.world.diplomatic_relation_get_value(rid_a) < state.world.diplomatic_relation_get_value(rid_b);
			};
			break;
		case diplomacy_list_sort::opinion:
			fn = [&](dcon::nation_id a, dcon::nation_id b) {
				auto grid_a = state.world.get_gp_relationship_by_gp_influence_pair(state.local_player_nation, a);
				auto grid_b = state.world.get_gp_relationship_by_gp_influence_pair(state.local_player_nation, b);
				if(!bool(grid_a) || !bool(grid_b))
					return a.index() < b.index();
				return state.world.gp_relationship_get_status(grid_a) < state.world.gp_relationship_get_status(grid_b);
			};
			break;
		// TODO: sorting for priority
		// TODO: sorting for each influence for each gp
		default:
			fn = [&](dcon::nation_id a, dcon::nation_id b) {
				dcon::nation_fat_id a_fat_id = dcon::fatten(state.world, a);
				auto a_name = text::produce_simple_string(state, a_fat_id.get_name());
				dcon::nation_fat_id b_fat_id = dcon::fatten(state.world, b);
				auto b_name = text::produce_simple_string(state, b_fat_id.get_name());
				return a_name < b_name;
			};
			break;
		}
		std::stable_sort(country_listbox->row_contents.begin(), country_listbox->row_contents.end(), [&](auto a, auto b) {
			bool r = fn(a, b);
			return sort_ascend ? r : !r;
		});
	}

	void filter_countries(sys::state& state, std::function<bool(dcon::nation_id)> filter_fun) {
		if(country_listbox) {
			country_listbox->row_contents.clear();
			state.world.for_each_nation([&](dcon::nation_id id) {
				if(state.world.nation_get_owned_province_count(id) != 0 && filter_fun(id))
					country_listbox->row_contents.push_back(id);
			});
			sort_countries(state);
			country_listbox->update(state);
		}
	}

	void filter_by_continent(sys::state& state, dcon::modifier_id mod_id) {
		filter_countries(state, [&](dcon::nation_id id) -> bool {
			dcon::nation_fat_id fat_id = dcon::fatten(state.world, id);
			auto cont_id = fat_id.get_capital().get_continent().id;
			return mod_id == cont_id;
		});
	}

	template<typename T>
	void add_action_button(sys::state& state, xy_pair offset) noexcept {
		auto ptr = make_element_by_type<T>(state, state.ui_state.defs_by_name.find("diplomacy_option")->second.definition);
		ptr->base_data.position = offset;
		action_buttons.push_back(ptr.get());
		add_child_to_front(std::move(ptr));
	}
public:
	void on_create(sys::state& state) noexcept override {
		generic_tabbed_window::on_create(state);
		set_visible(state, false);
		filter_countries(state, [](dcon::nation_id) { return true; });
		state.ui_state.diplomacy_subwindow = this;

		xy_pair base_gp_info_offset = state.ui_defs.gui[state.ui_state.defs_by_name.find("diplomacy_greatpower_pos")->second.definition].position;
		xy_pair gp_info_offset = base_gp_info_offset;
		for(uint8_t i = 0; i < uint8_t(state.defines.great_nations_count); i++) {
			auto ptr = make_element_by_type<diplomacy_greatpower_info>(state, state.ui_state.defs_by_name.find("diplomacy_greatpower_info")->second.definition);
			ptr->base_data.position = gp_info_offset;
			ptr->rank = i;
			// Increment gp offset
			gp_info_offset.y += ptr->base_data.size.y;
			if(i + 1 == uint8_t(state.defines.great_nations_count) / 2) {
				gp_info_offset.y = base_gp_info_offset.y;
				gp_info_offset.x += (ptr->base_data.size.x / 2) + 62; // Nudge
			}
			gp_infos.push_back(ptr.get());
			add_child_to_front(std::move(ptr));
		}

		// Fill out all the options for the diplomacy window
		xy_pair options_base_offset = state.ui_defs.gui[state.ui_state.defs_by_name.find("diplomacy_actions_pos")->second.definition].position;
		xy_pair options_size = state.ui_defs.gui[state.ui_state.defs_by_name.find("diplomacy_option")->second.definition].size;
		xy_pair options_offset = options_base_offset;
		add_action_button<diplomacy_action_window<diplomacy_action_ally_button>>(state, options_offset);
		options_offset.y += options_size.y;
		add_action_button<diplomacy_action_window<diplomacy_action_call_ally_button>>(state, options_offset);
		options_offset.y += options_size.y;
		add_action_button<diplomacy_action_window<diplomacy_action_military_access_button>>(state, options_offset);
		options_offset.y += options_size.y;
		add_action_button<diplomacy_action_window<diplomacy_action_give_military_access_button>>(state, options_offset);
		options_offset.y += options_size.y;
		add_action_button<diplomacy_action_window<diplomacy_action_increase_relations_button>>(state, options_offset);
		options_offset.y += options_size.y;
		add_action_button<diplomacy_action_window<diplomacy_action_decrease_relations_button>>(state, options_offset);
		options_offset.y += options_size.y;
		add_action_button<diplomacy_action_window<diplomacy_action_war_subisides_button>>(state, options_offset);
		options_offset.y += options_size.y;
		add_action_button<diplomacy_action_window<diplomacy_action_declare_war_button>>(state, options_offset);
		options_offset.y += options_size.y;
		add_action_button<diplomacy_action_window<diplomacy_action_command_units_button>>(state, options_offset);
		// Next row of actions...
		options_offset.x += options_size.x;
		options_offset.y = options_base_offset.y;
		add_action_button<diplomacy_action_window<diplomacy_action_discredit_button>>(state, options_offset);
		options_offset.y += options_size.y;
		add_action_button<diplomacy_action_window<diplomacy_action_expel_advisors_button>>(state, options_offset);
		options_offset.y += options_size.y;
		add_action_button<diplomacy_action_window<diplomacy_action_ban_embassy_button>>(state, options_offset);
		options_offset.y += options_size.y;
		add_action_button<diplomacy_action_window<diplomacy_action_increase_opinion_button>>(state, options_offset);
		options_offset.y += options_size.y;
		add_action_button<diplomacy_action_window<diplomacy_action_decrease_opinion_button>>(state, options_offset);
		options_offset.y += options_size.y;
		add_action_button<diplomacy_action_window<diplomacy_action_add_to_sphere_button>>(state, options_offset);
		options_offset.y += options_size.y;
		add_action_button<diplomacy_action_window<diplomacy_action_remove_from_sphere_button>>(state, options_offset);
		options_offset.y += options_size.y;
		add_action_button<diplomacy_action_window<diplomacy_action_justify_war_button>>(state, options_offset);

		auto new_win1 = make_element_by_type<diplomacy_action_dialog_window>(state, state.ui_state.defs_by_name.find("defaultdiplomacydialog")->second.definition);
		new_win1->set_visible(state, false);
		action_dialog_win = new_win1.get();
		add_child_to_front(std::move(new_win1));

		auto new_win2 = make_element_by_type<diplomacy_gp_action_dialog_window>(state, state.ui_state.defs_by_name.find("gpselectdiplomacydialog")->second.definition);
		new_win2->set_visible(state, false);
		gp_action_dialog_win = new_win2.get();
		add_child_to_front(std::move(new_win2));

		Cyto::Any payload = state.local_player_nation;
		impl_get(state, payload);
	}

	std::unique_ptr<element_base> make_child(sys::state& state, std::string_view name, dcon::gui_def_id id) noexcept override {
		if(name == "close_button") {
			return make_element_by_type<generic_close_button>(state, id);
		} else if(name == "gp_info") {
			auto ptr = make_element_by_type<generic_tab_button<diplomacy_window_tab>>(state, id);
			ptr->target = diplomacy_window_tab::great_powers;
			return ptr;
		} else if(name == "war_info") {
			auto ptr = make_element_by_type<generic_tab_button<diplomacy_window_tab>>(state, id);
			ptr->target = diplomacy_window_tab::wars;
			return ptr;
		} else if(name == "cb_info") {
			auto ptr = make_element_by_type<generic_tab_button<diplomacy_window_tab>>(state, id);
			ptr->target = diplomacy_window_tab::casus_belli;
			return ptr;
		} else if(name == "crisis_info") {
			auto ptr = make_element_by_type<generic_tab_button<diplomacy_window_tab>>(state, id);
			ptr->target = diplomacy_window_tab::crisis;
			return ptr;
		} else if(name == "filter_all") {
			auto ptr = make_element_by_type<generic_tab_button<country_list_filter>>(state, id);
			ptr->target = country_list_filter::all;
			return ptr;
		} else if(name == "cb_info_win") {
			auto ptr = make_element_immediate(state, id);
			casus_belli_window = ptr.get();
			ptr->set_visible(state, false);
			return ptr;
		} else if(name == "crisis_info_win") {
			auto ptr = make_element_immediate(state, id);
			crisis_window = ptr.get();
			ptr->set_visible(state, false);
			return ptr;
		} else if(name == "country_listbox") {
			auto ptr = make_element_by_type<diplomacy_country_listbox>(state, id);
			country_listbox = ptr.get();
			return ptr;
		} else if(name == "war_listbox") {
			auto ptr = make_element_by_type<diplomacy_war_listbox>(state, id);
			war_listbox = ptr.get();
			war_listbox->set_visible(state, false);
			return ptr;
		} else if(name == "diplomacy_country_facts") {
			auto ptr = make_element_by_type<diplomacy_country_facts>(state, id);
			country_facts = ptr.get();
			return ptr;
		} else if(name == "sort_by_boss") {
			auto ptr = make_element_by_type<diplomacy_sort_button<diplomacy_list_sort::boss>>(state, id);
			ptr->base_data.position.y -= 1; // Nudge
			return ptr;
		} else if(name == "sort_by_prestige") {
			auto ptr = make_element_by_type<diplomacy_sort_button<diplomacy_list_sort::prestige_rank>>(state, id);
			ptr->base_data.position.y -= 1; // Nudge
			return ptr;
		} else if(name == "sort_by_economic") {
			auto ptr = make_element_by_type<diplomacy_sort_button<diplomacy_list_sort::economic_rank>>(state, id);
			ptr->base_data.position.y -= 1; // Nudge
			return ptr;
		} else if(name == "sort_by_military") {
			auto ptr = make_element_by_type<diplomacy_sort_button<diplomacy_list_sort::military_rank>>(state, id);
			ptr->base_data.position.y -= 1; // Nudge
			return ptr;
		} else if(name == "sort_by_total") {
			auto ptr = make_element_by_type<diplomacy_sort_button<diplomacy_list_sort::total_rank>>(state, id);
			ptr->base_data.position.y -= 1; // Nudge
			return ptr;
		} else if(name == "sort_by_relation") {
			auto ptr = make_element_by_type<diplomacy_sort_button<diplomacy_list_sort::relation>>(state, id);
			ptr->base_data.position.y -= 1; // Nudge
			return ptr;
		} else if(name == "sort_by_opinion") {
			return make_element_by_type<diplomacy_sort_button<diplomacy_list_sort::opinion>>(state, id);
		} else if(name.substr(0, 14) == "sort_by_gpflag") {
			auto ptr = make_element_by_type<nation_gp_flag>(state, id);
			ptr->rank = uint16_t(std::stoi(std::string{ name.substr(14) }));
			ptr->base_data.position.y -= 2; // Nudge
			return ptr;
		} else if(name.length() >= 7 && name.substr(0, 7) == "filter_") {
			const auto filter_name = name.substr(7);
			auto ptr = make_element_by_type<generic_tab_button<dcon::modifier_id>>(state, id);
			ptr->target = ([&]() {
				dcon::modifier_id filter_mod_id{ 0 };
				auto it = state.key_to_text_sequence.find(parsers::lowercase_str(filter_name));
				if(it != state.key_to_text_sequence.end())
					state.world.for_each_modifier([&](dcon::modifier_id mod_id) {
						auto fat_id = dcon::fatten(state.world, mod_id);
						if(it->second == fat_id.get_name())
							filter_mod_id = mod_id;
					});
				return filter_mod_id;
			})();
			return ptr;
		} else {
			return nullptr;
		}
	}

	void hide_tabs(sys::state& state) {
		war_listbox->set_visible(state, false);
		casus_belli_window->set_visible(state, false);
		crisis_window->set_visible(state, false);
		for(auto e : gp_infos)
			e->set_visible(state, false);
	}

	message_result set(sys::state& state, Cyto::Any& payload) noexcept override {
		if(payload.holds_type<diplomacy_list_sort>()) {
			auto new_sort = any_cast<diplomacy_list_sort>(payload);
			sort_ascend = (new_sort == sort) ? !sort_ascend : true;
			sort = new_sort;
			sort_countries(state);
			country_listbox->update(state);
			return message_result::consumed;
		}
		return generic_tabbed_window<diplomacy_window_tab>::set(state, payload);
	}

	message_result get(sys::state& state, Cyto::Any& payload) noexcept override {
		if(payload.holds_type<diplomacy_window_tab>()) {
			auto enum_val = any_cast<diplomacy_window_tab>(payload);
			hide_tabs(state);
			switch(enum_val) {
			case diplomacy_window_tab::great_powers:
				for(auto e : gp_infos)
					e->set_visible(state, true);
				break;
			case diplomacy_window_tab::wars:
				war_listbox->set_visible(state, true);
				break;
			case diplomacy_window_tab::casus_belli:
				casus_belli_window->set_visible(state, true);
				break;
			case diplomacy_window_tab::crisis:
				crisis_window->set_visible(state, true);
				break;
			}
			active_tab = enum_val;
			return message_result::consumed;
		} else if(payload.holds_type<country_list_filter>()) {
			auto filter = any_cast<country_list_filter>(payload);
			switch(filter) {
			case country_list_filter::all:
				filter_countries(state, [](auto) { return true; });
				break;
			default:
				break;
			}
			return message_result::consumed;
		} else if(payload.holds_type<dcon::modifier_id>()) {
			auto mod_id = any_cast<dcon::modifier_id>(payload);
			filter_by_continent(state, mod_id);
			return message_result::consumed;
		} else if(payload.holds_type<dcon::nation_id>()) {
			for(const auto e : action_buttons)
				e->impl_set(state, payload);
			return country_facts->impl_set(state, payload);
		} else if(payload.holds_type<diplomacy_action>()) {
			auto v = any_cast<diplomacy_action>(payload);
			Cyto::Any new_payload = dcon::nation_id{};
			country_facts->impl_get(state, new_payload);

			gp_action_dialog_win->set_visible(state, false);
			action_dialog_win->set_visible(state, false);

			switch(v) {
			case diplomacy_action::discredit:
			case diplomacy_action::expel_advisors:
			case diplomacy_action::ban_embassy:
			case diplomacy_action::decrease_opinion:
				gp_action_dialog_win->set_visible(state, true);
				gp_action_dialog_win->impl_set(state, new_payload);
				gp_action_dialog_win->impl_set(state, payload);
				break;
			default:
				action_dialog_win->set_visible(state, true);
				action_dialog_win->impl_set(state, new_payload);
				action_dialog_win->impl_set(state, payload);
				break;
			}
			return message_result::consumed;
		}
		return generic_tabbed_window<diplomacy_window_tab>::get(state, payload);
	}
};

}

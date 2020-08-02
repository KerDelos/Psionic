
#include "ParsedGame.hpp"


map<string,ParsedGame::PreludeTokenType, ci_less> ParsedGame::to_prelude_token_type = {
	{"None", PreludeTokenType::None},
	{"Title", PreludeTokenType::Title},
	{"Author", PreludeTokenType::Author},
	{"Homepage", PreludeTokenType::Homepage},
	{"realtime_interval", PreludeTokenType::RealtimeInterval},
	{"Literal", PreludeTokenType::Literal},
};

map<string,ParsedGame::ObjectsTokenType, ci_less> ParsedGame::to_objects_token_type = {
	{"None", ObjectsTokenType::None},
	{"Literal", ObjectsTokenType::Literal},
	{"ColorName", ObjectsTokenType::ColorName},
	{"ColorHexCode", ObjectsTokenType::ColorHexCode},
	{"Pixel", ObjectsTokenType::Pixel},
};

map<string,ParsedGame::LegendTokenType, ci_less> ParsedGame::to_legend_token_type = {
	{"None", LegendTokenType::None},
	{"Identifier", LegendTokenType::Identifier},
	{"Return", LegendTokenType::Return},
	{"Equal", LegendTokenType::Equal},
	{"Or", LegendTokenType::Or},
	{"And", LegendTokenType::And},
};

map<string,ParsedGame::CollisionLayersTokenType, ci_less> ParsedGame::to_collision_layers_token_type = {
	{"None", CollisionLayersTokenType::None},
	{"Identifier", CollisionLayersTokenType::Identifier},
	{"Comma", CollisionLayersTokenType::Comma},
	{"Return", CollisionLayersTokenType::Return},
};

map<string,ParsedGame::RulesTokenType, ci_less> ParsedGame::to_rules_token_type = {
	{"None", RulesTokenType::None},
	{"LeftBracket", RulesTokenType::LeftBracket},
	{"RightBracket", RulesTokenType::RightBracket},
	{"Bar", RulesTokenType::Bar},
	{"RelativeRight", RulesTokenType::RelativeRight},
	{"RelativeLeft", RulesTokenType::RelativeLeft},
	{"RelativeUp", RulesTokenType::RelativeUp},
	{"RelativeDown", RulesTokenType::RelativeDown},
	{"Arrow", RulesTokenType::Arrow},
	{"Dots", RulesTokenType::Dots},
	{"Return", RulesTokenType::Return},
	{"NO", RulesTokenType::No},
	{"LATE", RulesTokenType::LATE},
	{"AGAIN", RulesTokenType::AGAIN},
	{"MOVING", RulesTokenType::MOVING},
	{"STATIONARY", RulesTokenType::STATIONARY},
    {"PARALLEL", RulesTokenType::PARALLEL},
    {"PERPENDICULAR", RulesTokenType::PERPENDICULAR},
    {"HORIZONTAL", RulesTokenType::HORIZONTAL},
    {"VERTICAL", RulesTokenType::VERTICAL},
	{"ORTHOGONAL", RulesTokenType::ORTHOGONAL},
	{"ACTION", RulesTokenType::ACTION},
	{"UP", RulesTokenType::UP},
	{"DOWN", RulesTokenType::DOWN},
	{"LEFT", RulesTokenType::LEFT},
	{"RIGHT", RulesTokenType::RIGHT},
	{"CHECKPOINT", RulesTokenType::CHECKPOINT},
	{"CANCEL", RulesTokenType::CANCEL},
	{"WIN", RulesTokenType::WIN},
	{"MESSAGE", RulesTokenType::MESSAGE},
	{"SFX0", RulesTokenType::SFX0},
	{"SFX1", RulesTokenType::SFX1},
	{"SFX2", RulesTokenType::SFX2},
	{"SFX3", RulesTokenType::SFX3},
	{"SFX4", RulesTokenType::SFX4},
	{"SFX5", RulesTokenType::SFX5},
	{"SFX6", RulesTokenType::SFX6},
	{"SFX7", RulesTokenType::SFX7},
	{"SFX8", RulesTokenType::SFX8},
	{"SFX9", RulesTokenType::SFX9},
	{"SFX10", RulesTokenType::SFX10},
	{"Identifier", RulesTokenType::Identifier},
};

map<string,ParsedGame::WinConditionsTokenType, ci_less> ParsedGame::to_win_conditions_token_type = {
	{"None", WinConditionsTokenType::None},
	{"Identifier", WinConditionsTokenType::Identifier},
	{"Return", WinConditionsTokenType::Return},
	{"NO", WinConditionsTokenType::NO},
	{"ALL", WinConditionsTokenType::ALL},
	{"ON", WinConditionsTokenType::ON},
	{"SOME", WinConditionsTokenType::SOME},
};

map<string,ParsedGame::LevelsTokenType, ci_less> ParsedGame::to_levels_token_type = {
	{"None", LevelsTokenType::None},
	{"MESSAGE", LevelsTokenType::MESSAGE},
	{"Return", LevelsTokenType::Return},
	{"MessageContent", LevelsTokenType::MessageContent},
	{"LevelTile", LevelsTokenType::LevelTile},
};

void ParsedGame::print_parsed_game(bool p_print_prelude,
        bool p_print_objects,
        bool p_print_legend,
        bool p_print_collision_layers,
        bool p_print_rules,
        bool p_print_win_conditions,
        bool p_print_levels) const
{
    if(p_print_prelude)
    {
        cout << "Parsed Prelude tokens :\n";
        for(auto token : prelude_tokens)
        {
            token.print(to_prelude_token_type);
        }
    }

    if(p_print_objects)
    {
        cout << "Parsed Objects tokens :\n";
        for(auto token : objects_tokens)
        {
            token.print(to_objects_token_type);
        }
    }

    if(p_print_legend)
    {
        cout << "Parsed Legend tokens :\n";
        for(auto token : legend_tokens)
        {
            token.print(to_legend_token_type);
        }
    }

    if(p_print_collision_layers)
    {
        cout << "Parsed collision layers tokens :\n";
        for(auto token : collision_layers_tokens)
        {
            token.print(to_collision_layers_token_type);
        }
    }

    if(p_print_rules)
    {
        cout << "Parsed rules tokens :\n";
        for(auto token : rules_tokens)
        {
            token.print(to_rules_token_type);
        }
    }

    if(p_print_win_conditions)
    {
        cout << "Parsed win conditions tokens :\n";
        for(auto token : win_conditions_tokens)
        {
            token.print(to_win_conditions_token_type);
        }
    }

    if(p_print_levels)
    {
        cout << "Parsed levels tokens :\n";
        for(auto token : levels_tokens)
        {
            token.print(to_levels_token_type);
        }
    }
}

#pragma once

#include <vector>
#include <map>
#include <string>
#include <iostream>

#include "EnumHelpers.hpp"

template<typename E>
struct Token{
	E token_type;
	string str_value;
	int token_line;

	Token(E p_token_type, string p_literal, int p_line)
	: token_type(p_token_type), str_value(p_literal), token_line(p_line){}

	void print(map<string,E, ci_less> p_enum_to_str) const {
		cout << "l." << token_line << " : " << enum_to_str(token_type,p_enum_to_str).value_or("ERROR") << " | literal : \""<< str_value<<"\"\n";
	}
};

struct ParsedGame{

    enum class PreludeTokenType{
        None,
        Title,
        Author,
        Homepage,
        Literal,
    };

    enum class ObjectsTokenType{
        None,
        Literal,
        ColorName,
        ColorHexCode,
        Pixel,
    };

    enum class LegendTokenType{
        None,
        Identifier,
        Return,
        Equal,
        Or,
        And,
    };

    enum class CollisionLayersTokenType{
        None,
        Identifier,
        Comma,
        Return,
    };

    enum class RulesTokenType{
        None,
        LeftBracket,
        RightBracket,
        Bar,
        RelativeRight,
        RelativeLeft,
        RelativeUp,
        RelativeDown,
        Arrow,
        Dots,
        Return,
        No,
        LATE,
        AGAIN,
        MOVING,
        STATIONARY,
        PARALLEL,
        PERPENDICULAR,
        HORIZONTAL,
        VERTICAL,
        ORTHOGONAL,
        ACTION,
        UP,
        DOWN,
        LEFT,
        RIGHT,
        CHECKPOINT,
        CANCEL,
        WIN,
        MESSAGE,
        SFX0,
        SFX1,
        SFX2,
        SFX3,
        SFX4,
        SFX5,
        SFX6,
        SFX7,
        SFX8,
        SFX9,
        SFX10,
        Identifier,
    };

    enum class WinConditionsTokenType{
        None,
        Identifier,
        Return,
        NO,
        ALL,
        ON,
        SOME,
    };

    enum class LevelsTokenType{
        None,
        MESSAGE,
        MessageContent,
        Return,
        LevelTile,
    };

    static map<string,PreludeTokenType, ci_less> to_prelude_token_type;
    static map<string,ObjectsTokenType, ci_less> to_objects_token_type;
    static map<string,LegendTokenType, ci_less> to_legend_token_type;
    static map<string,CollisionLayersTokenType, ci_less> to_collision_layers_token_type;
    static map<string,RulesTokenType, ci_less> to_rules_token_type;
    static map<string,WinConditionsTokenType, ci_less> to_win_conditions_token_type;
    static map<string,LevelsTokenType, ci_less> to_levels_token_type;

    vector<Token<PreludeTokenType>> prelude_tokens;
	vector<Token<ObjectsTokenType>> objects_tokens;
	vector<Token<LegendTokenType>> legend_tokens;
	//sounds tokens
	vector<Token<CollisionLayersTokenType>> collision_layers_tokens;
	vector<Token<RulesTokenType>> rules_tokens;
	vector<Token<WinConditionsTokenType>> win_conditions_tokens;
	vector<Token<LevelsTokenType>> levels_tokens;


    void print_parsed_game(
        bool p_print_prelude = false,
        bool p_print_objects= false,
        bool p_print_legend= false,
        bool p_print_collision_layers= false,
        bool p_print_rules = false,
        bool p_print_win_conditions = false,
        bool p_print_levels= false) const;
};

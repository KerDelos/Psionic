#pragma once

#include <string>
#include <iostream>
#include <fstream>
#include <vector>
#include <map>
#include <optional>
#include <memory>

#include "EnumHelpers.hpp"
#include "PSLogger.hpp"
#include "ParsedGame.hpp"
#include "TextProvider.hpp"

using namespace std;

enum class FileSection{
	None,
	Prelude,
	Objects,
	Legend,
	Sounds,
	CollisionLayers,
	Rules,
	WinConditions,
	Levels,
};

class Parser
{

public:
	static std::optional<ParsedGame> parse_from_file(string p_file_path, shared_ptr<PSLogger> p_logger);
	static std::optional<ParsedGame> parse_from_string(string p_text, shared_ptr<PSLogger> p_logger);
	static std::optional<ParsedGame> parse_from_text_provider(unique_ptr<TextProvider> p_text_provider, shared_ptr<PSLogger> p_logger);

protected:
	Parser(unique_ptr<TextProvider> p_text_provider, shared_ptr<PSLogger> p_logger);
	~Parser();

    std::optional<ParsedGame> parse_file();

	void try_change_file_section(FileSection p_new_file_section);

	//GENERAL PARSING FUNCTIONS


	void parse_comment(int p_comment_level = 0);
	string parse_word();
	void parse_equals_row();
	bool try_parse_return(bool consume = true);

	void detect_error(string p_error_msg);

	//PRELUDE PARSING FUNCTIONS
	void parse_prelude();
	void parse_prelude_identifier();
	void parse_prelude_literal();

	//OBJECTS PARSING FUNCTIONS
	void parse_objects();
	void parse_objects_color_hex_code();
	bool is_pixel(char p_char);

	//LEGEND PARSING FUNCTIONS
	void parse_legend();

	//SOUNDS PARSING FUNCTIONS
	void parse_sounds();

	//COLLISION LAYERS PARSING FUNCTIONS
	void parse_collision_layers();

	//RULES PARSING FUNCTIONS
	void parse_rules();
	void parse_rules_word();

	//WIN CONDITIONS PARSING FUNCTIONS
	void parse_win_conditions();

	//LEVELS PARSING FUNCTIONS
	void parse_levels();

protected:

	unique_ptr<TextProvider> m_text_provider;

	shared_ptr<PSLogger> m_logger;

	static const string m_parser_log_cat;

	FileSection m_current_file_section = FileSection::None;

	ParsedGame m_parsed_game;

	int m_line_counter = 1;

	bool m_has_error = false;
	bool m_parse_completed = false;

protected:

	template <class TokenType>
	bool try_parse_message(vector<Token<TokenType>>& tokens_array)
	{
		string temp_buffer = "";
		temp_buffer.push_back(m_text_provider->get_current_char());

		bool detected_word_end = false;
		while(m_text_provider->is_valid() && !detected_word_end)
		{
			m_text_provider->advance();
			if(try_parse_return(false))
			{
				detected_word_end = true;
			}
			else
			{
				switch(m_text_provider->get_current_char())
				{
					case ' ':
						detected_word_end = true;
					break;
					default:
						temp_buffer.push_back(m_text_provider->get_current_char());
					break;
				}
			}
		}

		ci_equal equal_comp;
		if(equal_comp("Message",temp_buffer))
		{
			//add token message and parse the rest
			tokens_array.push_back(Token<TokenType>(TokenType::MESSAGE,"",m_line_counter));

			temp_buffer.clear();

			detected_word_end = false;
			while(m_text_provider->is_valid() && !detected_word_end)
			{
				m_text_provider->advance();
				if(try_parse_return(false))
				{
					detected_word_end = true;
					m_text_provider->reverse();
				}
				else
				{
					temp_buffer.push_back(m_text_provider->get_current_char());
				}
			}
			tokens_array.push_back(Token<TokenType>(TokenType::MessageContent,temp_buffer,m_line_counter));

			return true;
		}
		else
		{
			//reverse everything and return false, we couldn't properly parse a message token
			for(auto it = temp_buffer.rbegin(); it != temp_buffer.rend(); it++)
			{
				m_text_provider->reverse();
			}
			return false;
		}
	}
};
